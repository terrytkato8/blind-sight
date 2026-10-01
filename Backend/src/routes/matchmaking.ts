import { Router } from 'express';
import { z } from 'zod';
import { pool } from '../db.js';
import { requirePlayer } from '../auth.js';

const router = Router();
router.use(requirePlayer);

const TicketBody = z.object({
  mode: z.enum(['blackout', 'manhunt']),
  region: z.string().min(1).max(32),
  partyId: z.string().max(64).optional(),
});

/** Queue for a match. One live ticket per player; re-queueing replaces the old one. */
router.post('/ticket', async (req, res) => {
  const parsed = TicketBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request' });
  const playerId = req.player!.playerId;

  await pool.query(
    `UPDATE matchmaking_tickets SET state = 'cancelled'
      WHERE player_id = $1 AND state = 'queued'`, [playerId]);

  const { rows } = await pool.query<{ ticket_id: string }>(
    `INSERT INTO matchmaking_tickets (player_id, mode, region, party_id)
     VALUES ($1, $2, $3, $4) RETURNING ticket_id`,
    [playerId, parsed.data.mode, parsed.data.region, parsed.data.partyId ?? null]);

  res.json({ ticketId: rows[0].ticket_id, state: 'queued' });
});

/** The client polls this every couple of seconds while queued. */
router.get('/ticket/:id', async (req, res) => {
  const { rows } = await pool.query(
    `SELECT t.state, t.match_id, t.mode, m.region, m.map_name, s.address
       FROM matchmaking_tickets t
       LEFT JOIN matches m ON m.match_id = t.match_id
       LEFT JOIN game_servers s ON s.server_id = m.server_id
      WHERE t.ticket_id = $1 AND t.player_id = $2`,
    [req.params.id, req.player!.playerId]);

  if (rows.length === 0) return res.status(404).json({ error: 'not_found' });
  const t = rows[0];

  // Only hand back an address once a server is actually allocated and reachable.
  if (t.state === 'matched' && t.address) {
    return res.json({
      state: 'matched',
      matchId: t.match_id,
      serverAddress: t.address,
      region: t.region,
      mode: t.mode,
      map: t.map_name,
    });
  }
  res.json({ state: t.state });
});

router.delete('/ticket/:id', async (req, res) => {
  await pool.query(
    `UPDATE matchmaking_tickets SET state = 'cancelled'
      WHERE ticket_id = $1 AND player_id = $2 AND state = 'queued'`,
    [req.params.id, req.player!.playerId]);
  res.json({ ok: true });
});

export default router;
