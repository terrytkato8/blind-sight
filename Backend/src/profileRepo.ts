import type pg from 'pg';
import { pool } from './db.js';

export interface ProfileRow {
  player_id: string; display_name: string; level: number; experience: number;
  currency: number; matches_played: number; rounds_as_hunter: number;
  total_catches: number; total_survivals: number;
  equipped_hunter_skin: string; equipped_hider_skin: string;
}

/** The single shape the game client receives. Keep this stable; the UE struct mirrors it. */
export async function loadProfile(playerId: string, client: pg.PoolClient | pg.Pool = pool) {
  const { rows } = await client.query<ProfileRow>(
    `SELECT player_id, display_name, level, experience, currency, matches_played,
            rounds_as_hunter, total_catches, total_survivals,
            equipped_hunter_skin, equipped_hider_skin
       FROM players WHERE player_id = $1`, [playerId]);
  if (!rows[0]) return null;
  const p = rows[0];

  const owned = await client.query<{ cosmetic_id: string }>(
    `SELECT cosmetic_id FROM player_cosmetics WHERE player_id = $1`, [playerId]);

  return {
    playerId: p.player_id,
    displayName: p.display_name,
    level: p.level,
    experience: p.experience,
    currency: p.currency,
    matchesPlayed: p.matches_played,
    roundsAsHunter: p.rounds_as_hunter,
    totalCatches: p.total_catches,
    totalSurvivals: p.total_survivals,
    equippedHunterSkin: p.equipped_hunter_skin,
    equippedHiderSkin: p.equipped_hider_skin,
    unlockedCosmetics: owned.rows.map((r) => r.cosmetic_id),
  };
}

/** Simple, legible curve: level N needs 1000*N XP. Tune freely; it is server-side only. */
export function levelForExperience(xp: number): number {
  let level = 1, needed = 1000;
  while (xp >= needed) { xp -= needed; level++; needed = 1000 * level; }
  return level;
}
