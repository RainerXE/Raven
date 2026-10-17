// Utilitaires partagés (le préfixe "_" empêche Pages d'en faire une route).
// L'authentification est entièrement déléguée à Clerk : ici on ne fait que vérifier
// le jeton de session avec le SDK officiel et lire les licences stockées chez Clerk.
import { createClerkClient, verifyToken } from "@clerk/backend";

export const json = (data, status = 200) =>
  new Response(JSON.stringify(data), {
    status,
    headers: { "content-type": "application/json", "cache-control": "no-store" },
  });

export const clerk = (env) => createClerkClient({ secretKey: env.CLERK_SECRET_KEY });

// Renvoie l'id Clerk de l'utilisateur connecté, ou null.
// Jeton : en-tête "Authorization: Bearer" (appels fetch) ; avec cookie:true, le cookie
// "__session" posé par Clerk est aussi accepté (utile pour un lien de téléchargement).
export async function authUser(request, env, { cookie = false } = {}) {
  let token = (request.headers.get("authorization") || "").replace(/^Bearer\s+/i, "");
  if (!token && cookie) {
    const m = (request.headers.get("cookie") || "").match(/(?:^|; )__session=([^;]+)/);
    token = m ? m[1] : "";
  }
  if (!token) return null;
  try {
    const r = await verifyToken(token, {
      secretKey: env.CLERK_SECRET_KEY,
      authorizedParties: [new URL(request.url).origin], // refuse les jetons émis pour un autre site
    });
    const payload = r && r.data ? r.data : r;
    return (payload && payload.sub) || null;
  } catch {
    return null;
  }
}

// Les licences sont dans les métadonnées publiques de l'utilisateur Clerk :
// lisibles par lui, modifiables uniquement côté serveur (webhook Stripe).
export const metaOf = (user) => user.publicMetadata || {};
export const licensesOf = (user) => (Array.isArray(metaOf(user).licenses) ? metaOf(user).licenses : []);
export const isActive = (l) => l.status === "active";
