import { Router } from 'express';
import { z } from 'zod';
import { pool } from '../db.js';
import { signPlayerToken, resolvePlatformAccount } from '../auth.js';
import { loadProfile } from '../profileRepo.js';

const router = Router();

const LoginBody = z.object({
  platform: z.enum(['eos', 'steam', 'dev']),
  idToken: z.string().min(1).max(4096),
});

/**
 * Exchange a platform identity token for a backend session token.
 * Creates the account on first sight — there is no separate registration step.
 */
router.post('/login', async (req, res) => {
  const parsed = LoginBody.safeParse(req.body);
  if (!parsed.success) return res.status(400).json({ error: 'bad_request' });

  let accountId: string | null;
  try {
    accountId = await resolvePlatformAccount(parsed.data.platform, parsed.data.idToken);
  } catch (e) {
    req.log.error({ e }, 'identity verification failed');
    return res.status(503).json({ error: 'identity_unavailable' });
  }
  if (!accountId) return res.status(401).json({ error: 'invalid_identity' });

  const { rows } = await pool.query<{ player_id: string; banned: boolean }>(
    `INSERT INTO players (platform, platform_account_id, display_name)
          VALUES ($1, $2, $3)
     ON CONFLICT (platform, platform_account_id)
       DO UPDATE SET last_seen_at = now()
     RETURNING player_id, banned`,
    [parsed.data.platform, accountId, `Player-${accountId.slice(0, 6)}`]);

  const player = rows[0];
  if (player.banned) return res.status(403).json({ error: 'banned' });

  // Everyone owns the default skins.
  await pool.query(
    `INSERT INTO player_cosmetics (player_id, cosmetic_id)
     SELECT $1, cosmetic_id FROM cosmetics WHERE price = 0
     ON CONFLICT DO NOTHING`, [player.player_id]);

  res.json({
    token: signPlayerToken(player.player_id),
    profile: await loadProfile(player.player_id),
  });
});

export default router;
