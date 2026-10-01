import jwt from 'jsonwebtoken';
import type { Request, Response, NextFunction } from 'express';
import { config, devAuthMode } from './config.js';

export interface PlayerClaims { playerId: string; }

declare global {
  // eslint-disable-next-line @typescript-eslint/no-namespace
  namespace Express {
    interface Request { player?: PlayerClaims; }
  }
}

export function signPlayerToken(playerId: string): string {
  return jwt.sign({ playerId }, config.jwtSecret, { expiresIn: '12h' });
}

/** Gate for routes a player may call. Never admits a server key. */
export function requirePlayer(req: Request, res: Response, next: NextFunction) {
  const header = req.header('authorization') ?? '';
  const token = header.startsWith('Bearer ') ? header.slice(7) : '';
  if (!token) return res.status(401).json({ error: 'missing_token' });

  try {
    req.player = jwt.verify(token, config.jwtSecret) as PlayerClaims;
    next();
  } catch {
    res.status(401).json({ error: 'invalid_token' });
  }
}

/**
 * Gate for /server/* routes. These can mint currency and write progression, so a
 * player token must never satisfy them — only the shared server key does.
 * Compared in constant time so the key can't be recovered by timing the response.
 */
export function requireServer(req: Request, res: Response, next: NextFunction) {
  const presented = req.header('x-server-key') ?? '';
  if (!config.serverKey) return res.status(503).json({ error: 'server_key_not_configured' });
  if (presented.length !== config.serverKey.length) return res.status(403).json({ error: 'forbidden' });

  let diff = 0;
  for (let i = 0; i < presented.length; i++) diff |= presented.charCodeAt(i) ^ config.serverKey.charCodeAt(i);
  if (diff !== 0) return res.status(403).json({ error: 'forbidden' });
  next();
}

/**
 * Turns a platform identity token into a stable account id.
 *
 * Production: verify the token against the platform. For EOS that means calling
 * Epic's token endpoint with the client credentials and reading the subject claim;
 * for Steam, validating the session ticket via the Web API. Until those are wired,
 * devAuthMode trusts the string, which is fine on a laptop and catastrophic in prod.
 */
export async function resolvePlatformAccount(platform: string, idToken: string): Promise<string | null> {
  if (devAuthMode) return idToken.trim() || null;

  if (platform === 'eos') {
    // TODO: POST to Epic's token verification endpoint with EOS_CLIENT_ID / SECRET,
    // confirm the audience matches EOS_PRODUCT_ID, and return the `sub` claim.
    throw new Error('EOS verification not implemented — configure or stay in dev auth mode');
  }
  if (platform === 'steam') {
    // TODO: ISteamUserAuth/AuthenticateUserTicket, confirm appid, return steamid.
    throw new Error('Steam verification not implemented');
  }
  return null;
}
