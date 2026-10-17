import { json, authUser, clerk, licensesOf, metaOf, isActive } from "../_lib.js";

export async function onRequestGet({ request, env }) {
  const uid = await authUser(request, env);
  if (!uid) return json({ error: "Sign in first." }, 401);
  const user = await clerk(env).users.getUser(uid);
  const licenses = licensesOf(user);
  let files = [];
  if (licenses.some(isActive)) {
    const list = await env.RELEASES.list({ prefix: "release/" });
    files = list.objects.filter((o) => o.key.length > 8).map((o) => ({ name: o.key.slice(8), size: o.size }));
  }
  return json({ licenses, files, billing: !!metaOf(user).stripeCustomer });
}
