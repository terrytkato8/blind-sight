import { Router } from 'express';
import { z } from 'zod';
import { pool } from '../db.js';
import { requirePlayer } from '../auth.js';
import { loadProfile } from '../profileRepo.js';

const router = Router();
router.use(requirePlayer);

router.get('/me', async (req, res) => {
  const profile = await loadProfile(req.player!.playerId);
  if (!profile) return res.status(404).json({ error: 'not_found' });
  res.json(profile);
});

const EquipBody = z.object({
  slot: z.enum(['hunter_skin', 'hider_skin']),
  cosmeticId: z.string().min(1).max(64),
});

/** Equipping is checked against ownership — a client claiming an unowned skin is rejected. */
router.post('/equip', async (req, res) => {
  const parsed = EquipBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request' });
  const { slot, cosmeticId } = parsed.data;
  const playerId = req.player!.playerId;

  const owned = await pool.query(
    `SELECT 1 FROM player_cosmetics pc
       JOIN cosmetics c USING (cosmetic_id)
      WHERE pc.player_id = $1 AND pc.cosmetic_id = $2 AND c.slot = $3 AND c.enabled`,
    [playerId, cosmeticId, slot]);
  if (owned.rowCount === 0) return res.status(403).json({ error: 'not_owned' });

  const column = slot === 'hunter_skin' ? 'equipped_hunter_skin' : 'equipped_hider_skin';
  await pool.query(`UPDATE players SET ${column} = $1 WHERE player_id = $2`, [cosmeticId, playerId]);

  res.json(await loadProfile(playerId));
});

export default router;
