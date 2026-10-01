import { Router } from 'express';
import { z } from 'zod';
import { pool, tx } from '../db.js';
import { requireServer } from '../auth.js';
import { levelForExperience } from '../profileRepo.js';

const router = Router();

// Every route below can write progression. The key check is the whole security boundary.
router.use(requireServer);

const RegisterBody = z.object({
  region: z.string().min(1).max(32),
  address: z.string().min(1).max(128),
  build: z.string().max(64).optional(),
});

router.post('/register', async (req, res) => {
  const parsed = RegisterBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request' });

  // Re-registering the same address replaces the old row, so a restarted
  // container does not leave a ghost server in the allocation pool.
  const { rows } = await pool.query<{ server_id: string }>(
    `INSERT INTO game_servers (region, address, build, state, last_heartbeat)
          VALUES ($1, $2, $3, 'idle', now())
     RETURNING server_id`,
    [parsed.data.region, parsed.data.address, parsed.data.build ?? null]);

  await pool.query(
    `DELETE FROM game_servers WHERE address = $1 AND server_id <> $2`,
    [parsed.data.address, rows[0].server_id]);

  res.json({ serverId: rows[0].server_id });
});

router.post('/heartbeat', async (req, res) => {
  const serverId = String(req.body?.serverId ?? '');
  const { rowCount } = await pool.query(
    `UPDATE game_servers SET last_heartbeat = now() WHERE server_id = $1`, [serverId]);
  if (rowCount === 0) return res.status(404).json({ error: 'unknown_server' });
  res.json({ ok: true });
});

router.post('/match-state', async (req, res) => {
  const { serverId, matchId, state, playerCount } = req.body ?? {};
  await tx(async (c) => {
    await c.query(
      `UPDATE game_servers SET state = $1, player_count = $2, last_heartbeat = now()
        WHERE server_id = $3`,
      [state === 'finished' ? 'idle' : state, Number(playerCount) || 0, serverId]);
    if (matchId) {
      await c.query(
        `UPDATE matches SET state = $1, ended_at = CASE WHEN $1 = 'finished' THEN now() ELSE ended_at END
          WHERE match_id = $2`, [state, matchId]);
    }
  });
  res.json({ ok: true });
});

/** Stops a player connecting to a server they were never matched into. */
router.post('/validate-player', async (req, res) => {
  const { playerId, matchId } = req.body ?? {};
  const { rowCount } = await pool.query(
    `SELECT 1 FROM match_players WHERE match_id = $1 AND player_id = $2`, [matchId, playerId]);
  res.status(rowCount ? 200 : 403).json({ ok: rowCount > 0 });
});

const ResultsBody = z.object({
  matchId: z.string().uuid(),
  serverId: z.string().uuid(),
  roundNumber: z.number().int().min(1),
  mode: z.string().max(32),
  entries: z.array(z.object({
    playerId: z.string().uuid(),
    role: z.enum(['hunter', 'hider']),
    points: z.number().int().min(0).max(100_000),
    placement: z.number().int().min(0),
    catches: z.number().int().min(0).max(32),
    survived: z.boolean(),
    survivalSeconds: z.number().min(0),
    shotsFired: z.number().int().min(0),
    throwablesUsed: z.number().int().min(0),
    distanceMeters: z.number().int().min(0).optional(),
  })).max(8),
});

/**
 * The only path by which currency and XP are created.
 *
 * Idempotent: match_rounds has a UNIQUE (match_id, round_number), so a server that
 * retries after a network blip cannot pay a round out twice.
 *
 * Note the clamps in the schema above — points are bounded even though the caller
 * is trusted, because a bug in the game server should not be able to mint a million
 * currency, and a leaked key should have a bounded blast radius.
 */
router.post('/round-results', async (req, res) => {
  const parsed = ResultsBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request', detail: parsed.error.issues });
  const { matchId, roundNumber, mode, entries } = parsed.data;

  try {
    await tx(async (c) => {
      const round = await c.query<{ id: string }>(
        `INSERT INTO match_rounds (match_id, round_number, mode) VALUES ($1, $2, $3)
         ON CONFLICT (match_id, round_number) DO NOTHING
         RETURNING id`,
        [matchId, roundNumber, mode]);

      if (round.rowCount === 0) return;   // already recorded; silently succeed

      const roundId = round.rows[0].id;
      for (const e of entries) {
        await c.query(
          `INSERT INTO round_results
             (round_id, player_id, role, points, placement, catches, survived, survival_secs, shots_fired, throwables_used)
           VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10)`,
          [roundId, e.playerId, e.role, e.points, e.placement, e.catches,
           e.survived, e.survivalSeconds, e.shotsFired, e.throwablesUsed]);

        const updated = await c.query<{ experience: number }>(
          `UPDATE players SET
             currency         = currency + $1,
             experience       = experience + $1,
             rounds_as_hunter = rounds_as_hunter + $2,
             total_catches    = total_catches + $3,
             total_survivals  = total_survivals + $4,
             last_seen_at     = now()
           WHERE player_id = $5
           RETURNING experience`,
          [e.points, e.role === 'hunter' ? 1 : 0, e.catches, e.survived ? 1 : 0, e.playerId]);

        if (updated.rows[0]) {
          await c.query(`UPDATE players SET level = $1 WHERE player_id = $2`,
            [levelForExperience(updated.rows[0].experience), e.playerId]);
        }
      }
    });
    res.json({ ok: true });
  } catch (e) {
    req.log.error({ e }, 'round results failed');
    res.status(500).json({ error: 'persist_failed' });
  }
});

const TelemetryBody = z.object({
  serverId: z.string().uuid().optional(),
  events: z.array(z.object({
    event: z.string().max(64),
    matchId: z.string().max(64).optional(),
    playerId: z.string().max(64).optional(),
    roundTime: z.number().optional(),
    numbers: z.record(z.number()).optional(),
    strings: z.record(z.string()).optional(),
  })).max(1000),
});

router.post('/telemetry', async (req, res) => {
  const parsed = TelemetryBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request' });

  // Single multi-row insert — a busy six-player round produces a few hundred events.
  const values: unknown[] = [];
  const tuples = parsed.data.events.map((e, i) => {
    const b = i * 6;
    values.push(e.event, e.matchId ?? null, e.playerId ?? null, e.roundTime ?? null,
                JSON.stringify(e.numbers ?? {}), JSON.stringify(e.strings ?? {}));
    return `($${b + 1},$${b + 2},$${b + 3},$${b + 4},$${b + 5}::jsonb,$${b + 6}::jsonb)`;
  });
  if (tuples.length === 0) return res.json({ ok: true, inserted: 0 });

  await pool.query(
    `INSERT INTO telemetry_events (event, match_id, player_id, round_time, numbers, strings)
     VALUES ${tuples.join(',')}`, values);

  res.json({ ok: true, inserted: tuples.length });
});

export default router;
