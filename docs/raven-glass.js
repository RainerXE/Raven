/* raven-glass.js — vraie réfraction pour .glass / .btn / .cap / .knob
 * Pour chaque élément : champ de déplacement calculé avec la loi de Snell
 * (bord convexe "squircle", indice n), injecté via un filtre SVG
 * feDisplacementMap utilisé en backdrop-filter. Chromium uniquement ;
 * ailleurs, le CSS retombe sur le simple flou (var(--rf) non défini). */
(function(root){
	var N = 1.5;            /* indice de réfraction du verre */
	var L = 128;            /* résolution de la table profil -> déplacement */

	/* hauteur de la surface en fonction de la profondeur t (0 = bord, 1 = fin du biseau) */
	function prof(t){ return Math.pow(1 - Math.pow(1 - t, 4), .25); }

	/* table : déplacement (px) d'un rayon vertical traversant le biseau à la profondeur t */
	function table(B){
		var lut = new Float32Array(L + 1), S = 1, i, e = .002;
		for (i = 0; i <= L; i++) {
			var t = i / L, t1 = Math.min(1, t + e), t0 = Math.max(0, t - e);
			var slope = (prof(t1) - prof(t0)) / (t1 - t0);            /* dh/dx, épaisseur = B */
			var a1 = Math.atan(slope), a2 = Math.asin(Math.sin(a1) / N); /* Snell */
			var d = Math.tan(a1 - a2) * (prof(t) * B + .35 * B);      /* décalage latéral */
			lut[i] = d; if (d > S) S = d;
		}
		return { lut: lut, S: S };
	}

	/* carte RGBA (R,G = déplacement encodé autour de 128) d'un rect arrondi w x h */
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
				R = 128 - nx * d * 127;        /* on échantillonne vers l'intérieur */
				G = 128 - ny * d * 127;
			}
			D[o] = R; D[o + 1] = G; D[o + 2] = 128; D[o + 3] = 255;
		}
		return { data: D, cw: cw, ch: ch, S: T.S };
	}

	if (typeof module !== "undefined") module.exports = { field: field, table: table, prof: prof };
	if (!root.document || !/Chrome\//.test(root.navigator.userAgent)) return;

	var doc = root.document, NS = "http://www.w3.org/2000/svg", XL = "http://www.w3.org/1999/xlink";
	var svg = doc.createElementNS(NS, "svg"), defs = doc.createElementNS(NS, "defs");
	svg.setAttribute("width", 0); svg.setAttribute("height", 0);
	svg.style.cssText = "position:absolute;pointer-events:none"; svg.appendChild(defs);
	var cache = {}, uid = 0;

	function radius(el, w, h){
		var v = root.getComputedStyle(el).borderTopLeftRadius.split(" ")[0];
		var r = v.slice(-1) === "%" ? parseFloat(v) / 100 * Math.min(w, h) : parseFloat(v);
		return Math.min(r || 0, w / 2, h / 2);
	}
	function filterFor(w, h, r, B){
		var key = [w, h, Math.round(r), Math.round(B)].join("x");
		if (cache[key]) return cache[key];
		var k = Math.min(1, Math.sqrt(120000 / (w * h)));            /* plafonne le nombre de pixels */
		var f = field(w, h, r, B, k), cv = doc.createElement("canvas");
		cv.width = f.cw; cv.height = f.ch;
		var cx = cv.getContext("2d"), im = cx.createImageData(f.cw, f.ch); im.data.set(f.data); cx.putImageData(im, 0, 0);
		var id = "rg" + (++uid), fl = doc.createElementNS(NS, "filter");
		fl.setAttribute("id", id); fl.setAttribute("filterUnits", "userSpaceOnUse");
		fl.setAttribute("color-interpolation-filters", "sRGB");
		fl.setAttribute("x", 0); fl.setAttribute("y", 0); fl.setAttribute("width", w); fl.setAttribute("height", h);
		var im2 = doc.createElementNS(NS, "feImage"), url = cv.toDataURL();
		im2.setAttribute("href", url); im2.setAttributeNS(XL, "xlink:href", url);
		im2.setAttribute("x", 0); im2.setAttribute("y", 0); im2.setAttribute("width", w); im2.setAttribute("height", h);
		im2.setAttribute("preserveAspectRatio", "none"); im2.setAttribute("result", "m");
		var dm = doc.createElementNS(NS, "feDisplacementMap");
		dm.setAttribute("in", "SourceGraphic"); dm.setAttribute("in2", "m");
		dm.setAttribute("scale", (f.S * 2).toFixed(2));
		dm.setAttribute("xChannelSelector", "R"); dm.setAttribute("yChannelSelector", "G");
		fl.appendChild(im2); fl.appendChild(dm); defs.appendChild(fl);
		return (cache[key] = id);
	}
	function update(el){
		var w = el.offsetWidth, h = el.offsetHeight;
		if (w < 12 || h < 12) return;
		var m = Math.min(w, h), B = Math.min(m / 2, Math.max(24, m * .14));
		el.style.setProperty("--rf", "url(#" + filterFor(w, h, radius(el, w, h), B) + ")");
	}
	function init(){
		doc.body.appendChild(svg);
		var ro = new root.ResizeObserver(function(es){ es.forEach(function(e){ update(e.target); }); });
		[].forEach.call(doc.querySelectorAll(".glass,.btn,.cap,.knob"), function(el){ update(el); ro.observe(el); });
	}
	if (doc.readyState === "loading") doc.addEventListener("DOMContentLoaded", init); else init();
})(typeof window !== "undefined" ? window : this);
