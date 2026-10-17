/*
 * npm test  (Node 20+): makes licenses with throw-away keys and checks the webhook signatures.
 * To check the licenses with the C tool as well:
 *   raven_license keygen ./test-keys
 *   node test/test.mjs ./test-keys      -> writes test-personal.raven-license, test-company.raven-license
 *   raven_license show test-personal.raven-license --keys ./test-keys     (signature: valid)
 */
import { readFileSync, writeFileSync } from "node:fs";
import { createHmac, generateKeyPairSync, randomBytes } from "node:crypto";
import { handleOrder, verifyWebhook } from "../src/worker.js";

let failures = 0;
const check = (ok, what) => { console.log((ok ? "ok   " : "FAIL ") + what); if (!ok) failures++; };

let signing, unlock;
const keys = process.argv[2];
if (keys) {
	signing = readFileSync(keys + "/signing.secret", "utf8");
	unlock = readFileSync(keys + "/unlock.key", "utf8");
} else {
	const { privateKey, publicKey } = generateKeyPairSync("ed25519");
	const d = Buffer.from(privateKey.export({ format: "jwk" }).d, "base64url");
	const x = Buffer.from(publicKey.export({ format: "jwk" }).x, "base64url");
	signing = Buffer.concat([d, x]).toString("hex");
	unlock = randomBytes(32).toString("hex");
}
const env = { RAVEN_SIGNING_SECRET: signing, RAVEN_UNLOCK_KEY: unlock, PRODUCT_PERSONAL: "prod_personal", PRODUCT_COMPANY: "prod_company" };
const sent = [];
const send = async (_env, mail) => { sent.push(mail); };

check((await handleOrder({ id: "o1", product_id: "prod_personal", customer: { email: "jane@example.com", name: "Jane Doe" } }, env, send)).startsWith("sent personal"), "personal order");
check(/expires: never\n/.test(sent[0].license), "personal never expires");
check((await handleOrder({ id: "o2", product_id: "prod_company", customer: { email: "dev@xyz.com", name: "Studio XYZ" },
	subscription: { current_period_end: "2026-11-17T05:00:00Z" } }, env, send)).startsWith("sent company"), "company order");
check(/expires: 2026-11-22\n/.test(sent[1].license), "company: end of the paid month + 5 days");
check((await handleOrder({ id: "o3", product_id: "other" }, env, send)).startsWith("ignored"), "other products are ignored");
check(/^Raven license 1\nname: .+\nemail: .+\ntype: .+\nissued: \d{4}-\d\d-\d\d\nexpires: .+\nid: [0-9a-f]{32}\nunlock: [0-9a-f]{64}\nsignature: [0-9a-f]{128}\n$/.test(sent[0].license), "format of raven_license issue");
if (keys) {
	writeFileSync("test-personal.raven-license", sent[0].license);
	writeFileSync("test-company.raven-license", sent[1].license);
}

const body = '{"type":"order.paid"}', id = "msg_1", ts = String(Math.floor(Date.now() / 1000));
for (const [secret, raw, style] of [
	["whsec_oldStyle", Buffer.from("whsec_oldStyle"), "Polar HMAC"],
	["whsec_" + Buffer.from("0123456789abcdef").toString("base64"), Buffer.from("0123456789abcdef"), "Standard Webhooks"]]) {
	const sig = createHmac("sha256", raw).update(id + "." + ts + "." + body).digest("base64");
	const headers = new Headers({ "webhook-id": id, "webhook-timestamp": ts, "webhook-signature": "v1," + sig });
	check(await verifyWebhook(headers, body, secret), "webhook signature (" + style + ")");
	check(!(await verifyWebhook(headers, body + " ", secret)), "a changed body is refused (" + style + ")");
}
process.exit(failures ? 1 : 0);
