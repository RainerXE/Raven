import { json, authUser, clerk, licensesOf, isActive } from "../_lib.js";

// Les binaires "Release" vivent dans R2 (préfixe release/), jamais en accès public.
export async function onRequestGet({ request, env }) {
  const uid = await authUser(request, env, { cookie: true });
  if (!uid) return json({ error: "Sign in first." }, 401);
  const user = await clerk(env).users.getUser(uid);
  if (!licensesOf(user).some(isActive)) return json({ error: "No active license." }, 403);

  const name = new URL(request.url).searchParams.get("file") || "";
  if (!/^[\w.\-]{1,100}$/.test(name)) return json({ error: "Bad file name." }, 400);
  const obj = await env.RELEASES.get("release/" + name);
  if (!obj) return json({ error: "File not found." }, 404);
  return new Response(obj.body, {
    headers: {
      "content-type": "application/octet-stream",
      "content-disposition": `attachment; filename="${name}"`,
      "content-length": String(obj.size),
      "cache-control": "private, no-store",
    },
  });
}
