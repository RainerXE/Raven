import Stripe from "stripe";
import { clerk, licensesOf, metaOf } from "../_lib.js";

const ok = () => new Response("ok");
const nowSec = () => Math.floor(Date.now() / 1000);

export async function onRequestPost({ request, env }) {
  const stripe = new Stripe(env.STRIPE_SECRET_KEY, { httpClient: Stripe.createFetchHttpClient() });
  let ev;
  try {
    // Vérification de signature faite par le SDK officiel Stripe.
    ev = await stripe.webhooks.constructEventAsync(
      await request.text(),
      request.headers.get("stripe-signature") || "",
      env.STRIPE_WEBHOOK_SECRET,
      undefined,
      Stripe.createSubtleCryptoProvider()
    );
  } catch {
    return new Response("bad signature", { status: 400 });
  }
  const c = clerk(env);
  const o = ev.data.object;

  if (ev.type === "checkout.session.completed" || ev.type === "checkout.session.async_payment_succeeded") {
    if (o.payment_status !== "paid" && o.payment_status !== "no_payment_required") return ok();
    // client_reference_id = id Clerk de l'utilisateur, ajouté au lien de paiement par license.html
    const uid = o.client_reference_id;
    if (!uid) { console.warn("Stripe session without client_reference_id:", o.id); return ok(); }
    let user;
    try { user = await c.users.getUser(uid); } catch { console.warn("Unknown Clerk user:", uid); return ok(); }

    const licenses = licensesOf(user);
    if (!licenses.some((l) => l.session === o.id)) {            // idempotent
      licenses.push({
        kind: o.mode === "subscription" ? "company" : "personal",
        status: "active",
        created: nowSec(),
        session: o.id,
        subscription: o.subscription || null,
      });
    }
    await c.users.updateUserMetadata(uid, {
      publicMetadata: { licenses, stripeCustomer: o.customer || metaOf(user).stripeCustomer || null },
    });
    // Pour retrouver l'utilisateur lors des événements d'abonnement ultérieurs.
    if (o.customer) await stripe.customers.update(o.customer, { metadata: { clerk_user_id: uid } });
  } else if (ev.type === "customer.subscription.updated" || ev.type === "customer.subscription.deleted") {
    const live = o.status === "active" || o.status === "trialing";
    const status = ev.type === "customer.subscription.deleted" ? "canceled" : live ? "active" : "inactive";
    const cust = await stripe.customers.retrieve(o.customer);
    const uid = cust && !cust.deleted && cust.metadata && cust.metadata.clerk_user_id;
    if (!uid) return ok();
    const user = await c.users.getUser(uid);
    const licenses = licensesOf(user).map((l) => (l.subscription === o.id ? { ...l, status } : l));
    await c.users.updateUserMetadata(uid, { publicMetadata: { licenses } });
  }
  return ok();
}
