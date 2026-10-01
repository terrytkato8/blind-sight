import { tx, pool } from './db.js';
import { config } from './config.js';

/**
 * Forms matches from queued tickets and allocates an idle server to each.
 *
 * Deliberately simple: group by (mode, region), take the oldest N tickets, find an
 * idle server in that region that has heartbeat recently. No skill rating — Blind
 * Sight rotates the Hunter role every round, so lobby balance matters far less than
 * it would in a fixed-role game. Add rating later if playtests show a need.
 *
 * Runs every 2s from index.ts. Everything happens in one transaction so two ticks
 * cannot allocate the same server twice.
 */
export async function runMatchmaker(): Promise<void> {
  const groups = await pool.query<{ mode: string; region: string; waiting: string }>(
    `SELECT mode, region, COUNT(*) AS waiting
       FROM matchmaking_tickets
      WHERE state = 'queued'
      GROUP BY mode, region
     HAVING COUNT(*) >= $1`, [config.matchMinSize]);

  for (const g of groups.rows) {
    await tx(async (c) => {
      const tickets = await c.query<{ ticket_id: string; player_id: string }>(
        `SELECT ticket_id, player_id FROM matchmaking_tickets
          WHERE state = 'queued' AND mode = $1 AND region = $2
          ORDER BY created_at
          LIMIT $3
          FOR UPDATE SKIP LOCKED`,
        [g.mode, g.region, config.matchSize]);

      if (tickets.rowCount! < config.matchMinSize) return;

      // An idle server that has checked in within the last 45 seconds.
      const server = await c.query<{ server_id: string }>(
        `SELECT server_id FROM game_servers
          WHERE region = $1 AND state = 'idle' AND last_heartbeat > now() - interval '45 seconds'
          ORDER BY last_heartbeat DESC
          LIMIT 1
          FOR UPDATE SKIP LOCKED`, [g.region]);

      if (server.rowCount === 0) return;   // no capacity; tickets stay queued

      const mapName = g.mode === 'manhunt' ? 'L_Slice01' : 'L_Slice01';
      const match = await c.query<{ match_id: string }>(
        `INSERT INTO matches (mode, region, map_name, server_id, state)
         VALUES ($1, $2, $3, $4, 'allocating') RETURNING match_id`,
        [g.mode, g.region, mapName, server.rows[0].server_id]);
      const matchId = match.rows[0].match_id;

      for (const t of tickets.rows) {
        await c.query(
          `INSERT INTO match_players (match_id, player_id) VALUES ($1, $2) ON CONFLICT DO NOTHING`,
          [matchId, t.player_id]);
        await c.query(
          `UPDATE matchmaking_tickets SET state = 'matched', match_id = $1 WHERE ticket_id = $2`,
          [matchId, t.ticket_id]);
      }

      await c.query(
        `UPDATE game_servers SET state = 'lobby', current_match = $1 WHERE server_id = $2`,
        [matchId, server.rows[0].server_id]);
    });
  }

  // Reap servers that stopped reporting — a crashed container must not hold an allocation.
  await pool.query(
    `DELETE FROM game_servers WHERE last_heartbeat < now() - interval '2 minutes'`);
  await pool.query(
    `UPDATE matchmaking_tickets SET state = 'failed'
      WHERE state = 'queued' AND created_at < now() - interval '5 minutes'`);
}
