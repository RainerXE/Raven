/* raven-glass.js — réfraction "liquid glass" pour .glass / .btn / .cap / .knob (Chromium)
 * Filtre SVG par élément : déplacement par loi de Snell sur biseau squircle, utilisé en backdrop-filter.
 * Perf : 1 seule passe de déplacement (3 avec dispersion, seulement sur les petits éléments),
 * cartes basse résolution mises en cache, filtres créés à l'approche du viewport, recyclés quand
 * plus utilisés, recalcul différé pendant les redimensionnements. Ailleurs : flou CSS seul. */
(function(root){
	var N = 1.5, L = 128;
	var DISP = .07;          /* dispersion chromatique ; 0 = off */
	var DISP_MAX = 60000;    /* aire max (px²) pour activer la dispersion */
	var PIX = 40000;         /* pixels max d'une carte de déplacement */

	function prof(t){ return Math.pow(1 - Math.pow(1 - t, 4), .25); }
	function table(B){
		var lut = new Float32Array(L + 1), S = 1, i, e = .002;
		for (i = 0; i <= L; i++) {
			var t = i / L, t1 = Math.min(1, t + e), t0 = Math.max(0, t - e);
			var a1 = Math.atan((prof(t1) - prof(t0)) / (t1 - t0)), a2 = Math.asin(Math.sin(a1) / N);
			var d = Math.tan(a1 - a2) * (prof(t) * B + .35 * B);
			lut[i] = d; if (d > S) S = d;
		}
		return { lut: lut, S: S };
	}
	function field(w, h, r, B, k){
		var cw = Math.max(1, Math.round(w * k)), ch = Math.max(1, Math.round(h * k));
		var T = table(B), D = new Uint8ClampedArray(cw * ch * 4);
		var hx = w / 2, hy = h / 2, rr = Math.min(r, hx, hy), x, y;
		for (y = 0; y < ch; y++) for (x = 0; x < cw; x++) {
			var px = (x + .5) / k - hx, py = (y + .5) / k - hy;
			var qx = Math.abs(px) - (hx - rr), qy = Math.abs(py) - (hy - rr), nx, ny, sd;
			var sx = px < 0 ? -1 : 1, sy = py < 0 ? -1 : 1;
			if (qx > 0 && qy > 0) { var l = Math.sqrt(qx * qx + qy * qy); sd = l - rr; nx = qx / l * sx; ny = qy / l * sy; }
			else if (qx > qy) { sd = qx - rr; nx = sx; ny = 0; }
			else { sd = qy - rr; nx = 0; ny = sy; }
			var dep = -sd, o = (y * cw + x) * 4, R = 128, G = 128;
			if (dep >= 0 && dep < B) {
				var d = T.lut[Math.round(dep / B * L)] / T.S;
				R = 128 - nx * d * 127; G = 128 - ny * d * 127;
			}
			D[o] = R; D[o + 1] = G; D[o + 2] = 128; D[o + 3] = 255;
		}
		return { data: D, cw: cw, ch: ch, S: T.S };
	}
	if (typeof module !== "undefined") module.exports = { field: field, table: table, prof: prof };

	var nav = root.navigator;
	if (!root.document || !/Chrome\//.test(nav.userAgent)) return;
	if ((nav.deviceMemory && nav.deviceMemory <= 2) || root.matchMedia("(prefers-reduced-transparency:reduce)").matches) return;

	var doc = root.document, NS = "http://www.w3.org/2000/svg", XL = "http://www.w3.org/1999/xlink";
	var svg = doc.createElementNS(NS, "svg"), defs = doc.createElementNS(NS, "defs");
	svg.setAttribute("width", 0); svg.setAttribute("height", 0);
	svg.style.cssText = "position:absolute;pointer-events:none"; svg.appendChild(defs);
	var maps = {}, filters = {}, uid = 0;
	var CH = [["r", 1 - DISP, "1 0 0 0 0  0 0 0 0 0  0 0 0 0 0  0 0 0 1 0"],
	          ["g", 1,        "0 0 0 0 0  0 1 0 0 0  0 0 0 0 0  0 0 0 1 0"],
	          ["b", 1 + DISP, "0 0 0 0 0  0 0 0 0 0  0 0 1 0 0  0 0 0 1 0"]];

	function fe(name, a){ var e = doc.createElementNS(NS, name); for (var k in a) e.setAttribute(k, a[k]); return e; }
	function radius(el, w, h){
		var v = root.getComputedStyle(el).borderTopLeftRadius.split(" ")[0];
		var r = v.slice(-1) === "%" ? parseFloat(v) / 100 * Math.min(w, h) : parseFloat(v);
		return Math.min(r || 0, w / 2, h / 2);
	}
	/* carte PNG partagée entre toutes les tailles proches (w,h arrondis à 8 px) */
	function mapFor(w, h, r, B){
		var qw = Math.max(8, Math.round(w / 8) * 8), qh = Math.max(8, Math.round(h / 8) * 8);
		var key = [qw, qh, Math.round(r), Math.round(B)].join("x");
		if (maps[key]) return maps[key];
		var f = field(qw, qh, r, B, Math.min(1, Math.sqrt(PIX / (qw * qh))));
		var c = doc.createElement("canvas"); c.width = f.cw; c.height = f.ch;
		var cx = c.getContext("2d"), im = cx.createImageData(f.cw, f.ch); im.data.set(f.data); cx.putImageData(im, 0, 0);
		return (maps[key] = { url: c.toDataURL(), S: f.S });
	}
	function acquire(w, h, r, B){
		var disp = DISP && w * h <= DISP_MAX, key = [w, h, Math.round(r), Math.round(B), disp ? 1 : 0].join("x");
		var e = filters[key];
		if (e) { e.n++; return e; }
		var m = mapFor(w, h, r, B), id = "rg" + (++uid);
		var fl = fe("filter", { id: id, filterUnits: "userSpaceOnUse", "color-interpolation-filters": "sRGB", x: 0, y: 0, width: w, height: h });
		var im = fe("feImage", { x: 0, y: 0, width: w, height: h, preserveAspectRatio: "none", result: "m", href: m.url });
		im.setAttributeNS(XL, "xlink:href", m.url); fl.appendChild(im);
		if (!disp) {
			fl.appendChild(fe("feDisplacementMap", { "in": "SourceGraphic", in2: "m", scale: (m.S * 2).toFixed(2), xChannelSelector: "R", yChannelSelector: "G" }));
		} else {
			CH.forEach(function(c){
				fl.appendChild(fe("feDisplacementMap", { "in": "SourceGraphic", in2: "m", scale: (m.S * 2 * c[1]).toFixed(2), xChannelSelector: "R", yChannelSelector: "G", result: "d" + c[0] }));
				fl.appendChild(fe("feColorMatrix", { "in": "d" + c[0], type: "matrix", values: c[2], result: "c" + c[0] }));
			});
			fl.appendChild(fe("feBlend", { "in": "cr", in2: "cg", mode: "screen", result: "rg" }));
			fl.appendChild(fe("feBlend", { "in": "rg", in2: "cb", mode: "screen" }));
		}
		defs.appendChild(fl);
		return (filters[key] = { id: id, n: 1, node: fl, key: key });
	}
	function release(e){ if (e && --e.n <= 0) { defs.removeChild(e.node); delete filters[e.key]; } }

	function update(el){
		var w = el.offsetWidth, h = el.offsetHeight;
		if (w < 12 || h < 12) return;
		var m = Math.min(w, h), B = Math.min(m / 2, Math.max(22, Math.min(m * .16, 44)));
		var next = acquire(w, h, radius(el, w, h), B), prev = el._rg;
		if (prev === next) { next.n--; return; }
		el._rg = next; el.style.setProperty("--rf", "url(#" + next.id + ")"); release(prev);
	}

	function init(){
		doc.body.appendChild(svg);
		var timers = new WeakMap();
		/* redimensionnement : recalcul différé (évite une carte par frame d'animation) */
		var ro = new root.ResizeObserver(function(es){
			es.forEach(function(e){
				var el = e.target; clearTimeout(timers.get(el));
				timers.set(el, setTimeout(function(){ update(el); }, 120));
			});
		});
		/* création à l'approche du viewport seulement */
		var io = new root.IntersectionObserver(function(es){
			es.forEach(function(e){
				if (!e.isIntersecting) return;
				io.unobserve(e.target); update(e.target); ro.observe(e.target);
			});
		}, { rootMargin: "300px" });
		[].forEach.call(doc.querySelectorAll(".glass,.btn,.cap,.knob"), function(el){ io.observe(el); });
	}
	if (doc.readyState === "loading") doc.addEventListener("DOMContentLoaded", init); else init();
})(typeof window !== "undefined" ? window : this);
