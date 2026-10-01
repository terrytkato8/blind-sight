import { Router } from 'express';
import { z } from 'zod';
import { tx } from '../db.js';
import { requirePlayer } from '../auth.js';
import { loadProfile } from '../profileRepo.js';

const router = Router();
router.use(requirePlayer);

const PurchaseBody = z.object({ cosmeticId: z.string().min(1).max(64) });

/**
 * Price and balance are read inside the transaction with a row lock, so two
 * simultaneous purchases cannot both pass the affordability check.
 * The client never sends a price.
 */
router.post('/purchase', async (req, res) => {
  const parsed = PurchaseBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request' });
  const playerId = req.player!.playerId;

  try {
    const profile = await tx(async (c) => {
      const item = await c.query<{ price: number }>(
        `SELECT price FROM cosmetics WHERE cosmetic_id = $1 AND enabled`, [parsed.data.cosmeticId]);
      if (item.rowCount === 0) throw Object.assign(new Error('no_such_item'), { status: 404 });

      const already = await c.query(
        `SELECT 1 FROM player_cosmetics WHERE player_id = $1 AND cosmetic_id = $2`,
        [playerId, parsed.data.cosmeticId]);
      if (already.rowCount! > 0) throw Object.assign(new Error('already_owned'), { status: 409 });

      const wallet = await c.query<{ currency: number }>(
        `SELECT currency FROM players WHERE player_id = $1 FOR UPDATE`, [playerId]);
      const price = item.rows[0].price;
      if (wallet.rows[0].currency < price) throw Object.assign(new Error('insufficient'), { status: 402 });

      await c.query(`UPDATE players SET currency = currency - $1 WHERE player_id = $2`, [price, playerId]);
      await c.query(`INSERT INTO player_cosmetics (player_id, cosmetic_id) VALUES ($1, $2)`,
        [playerId, parsed.data.cosmeticId]);

      return loadProfile(playerId, c);
    });
    res.json(profile);
  } catch (e: any) {
    res.status(e.status ?? 500).json({ error: e.message ?? 'error' });
  }
});

router.get('/catalog', async (_req, res) => {
  const { pool } = await import('../db.js');
  const { rows } = await pool.query(
    `SELECT cosmetic_id, slot, display_name, price FROM cosmetics WHERE enabled ORDER BY price`);
  res.json(rows);
});

export default router;
