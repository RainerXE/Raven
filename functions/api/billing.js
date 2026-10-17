import Stripe from "stripe";
import { json, authUser, clerk, metaOf } from "../_lib.js";

// Portail client Stripe : factures, carte, annulation de l'abonnement entreprise.
// Jeton Bearer obligatoire (pas de cookie) : un autre site ne peut pas déclencher cet appel.
export async function onRequestPost({ request, env }) {
  const uid = await authUser(request, env);
  if (!uid) return json({ error: "Sign in first." }, 401);
  const customer = metaOf(await clerk(env).users.getUser(uid)).stripeCustomer;
  if (!customer) return json({ error: "No billing profile yet." }, 404);
  const stripe = new Stripe(env.STRIPE_SECRET_KEY, { httpClient: Stripe.createFetchHttpClient() });
  const s = await stripe.billingPortal.sessions.create({
    customer,
    return_url: new URL("/account.html", request.url).href,
  });
  return json({ url: s.url });
}
