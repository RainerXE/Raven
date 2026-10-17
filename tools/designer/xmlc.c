/**
 * Raven Designer - xmlc.c
 * An .xmlc file is cut into its pieces: the text between them (comments,
 * blank lines) is kept as it is, each <script> goes to a C tab, the
 * interface to the visual editor. Saving puts the pieces back in order.
 */

#include "ide.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { PIECE_TEXT, PIECE_SCRIPT, PIECE_INTERFACE } piece_kind_t;

typedef struct piece_t {
	piece_kind_t kind;
	/* text: all of it; script: the opening tag ("<script>"), the code, the closing tag */
	char* before;
	char* body;
	char* after;
	int body_line;   /* line of the first character of the body in the file (1-based) */
	Rid editor;      /* script: its tab */
} piece_t;

static struct {
	char* path;
	piece_t* pieces;
	int count;
} g_x;

static void free_pieces(void) {
	int i;
	for (i = 0; i < g_x.count; i++) {
		free(g_x.pieces[i].before);
		free(g_x.pieces[i].body);
		free(g_x.pieces[i].after);
	}
	free(g_x.pieces);
	g_x.pieces = NULL;
	g_x.count = 0;
}

static piece_t* add_piece(piece_kind_t kind) {
	piece_t* piece;
	g_x.pieces = (piece_t*)realloc(g_x.pieces, sizeof(piece_t) * (size_t)(g_x.count + 1));
	piece = &g_x.pieces[g_x.count++];
	memset(piece, 0, sizeof(*piece));
	piece->kind = kind;
	piece->editor = R_INVALID_ID;
	return piece;
}

static char* slice(const char* a, const char* b) {
	char* out = (char*)malloc((size_t)(b - a) + 1);
	memcpy(out, a, (size_t)(b - a));
	out[b - a] = 0;
	return out;
}

static int line_of(const char* text, const char* at) {
	int line = 1;
	for (; text < at; text++) if (*text == '\n') line++;
	return line;
}

static int starts_i(const char* p, const char* end, const char* s) {
	size_t n = strlen(s), i;
	if ((size_t)(end - p) < n) return 0;
	for (i = 0; i < n; i++) if (tolower((unsigned char)p[i]) != tolower((unsigned char)s[i])) return 0;
	return 1;
}

/* After the end of the element that starts at p ('<'), or NULL. */
static const char* skip_element(const char* p, const char* end) {
	int depth = 0;
	while (p < end) {
		if (starts_i(p, end, "<!--")) {
			const char* close = strstr(p + 4, "-->");
			if (!close) return NULL;
			p = close + 3;
		} else if (starts_i(p, end, "<![CDATA[")) {
			const char* close = strstr(p + 9, "]]>");
			if (!close) return NULL;
			p = close + 3;
		} else if (*p == '<' && p + 1 < end && p[1] == '/') {
			const char* close = memchr(p, '>', (size_t)(end - p));
			if (!close) return NULL;
			p = close + 1;
			if (--depth == 0) return p;
		} else if (*p == '<') {
			/* an opening tag: its attributes may hold '>' in quotes */
			char quote = 0;
			const char* q = p + 1;
			for (; q < end; q++) {
				if (quote) { if (*q == quote) quote = 0; }
				else if (*q == '"' || *q == '\'') quote = *q;
				else if (*q == '>') break;
			}
			if (q >= end) return NULL;
			if (q[-1] != '/') depth++;
			p = q + 1;
			if (depth == 0) return p;
		} else {
			p++;
		}
	}
	return NULL;
}

static int parse(const char* text) {
	const char* p = text;
	const char* end = text + strlen(text);
	const char* plain = p;
	int has_interface = 0;
	free_pieces();
	while (p < end) {
		if (starts_i(p, end, "<!--")) {
			const char* close = strstr(p + 4, "-->");
			p = close ? close + 3 : end;
		} else if (starts_i(p, end, "<?") || starts_i(p, end, "<!")) {
			const char* close = memchr(p, '>', (size_t)(end - p));
			p = close ? close + 1 : end;
		} else if (starts_i(p, end, "<script") && (p + 7 >= end || isspace((unsigned char)p[7]) || p[7] == '>' || p[7] == '/')) {
			const char* open_end = memchr(p, '>', (size_t)(end - p));
			const char* close;
			piece_t* piece;
			if (!open_end) return 0;
			if (p != plain) add_piece(PIECE_TEXT)->before = slice(plain, p);
			piece = add_piece(PIECE_SCRIPT);
			piece->before = slice(p, open_end + 1);
			if (open_end[-1] == '/') {
				/* <script/> */
				piece->body = ide_strdup("");
				piece->after = ide_strdup("");
				piece->body_line = line_of(text, open_end + 1);
				p = plain = open_end + 1;
				continue;
			}
			for (close = open_end + 1; close < end && !starts_i(close, end, "</script"); close++) {}
			if (close >= end) return 0;
			piece->body = slice(open_end + 1, close);
			piece->body_line = line_of(text, open_end + 1);
			{
				const char* after_end = memchr(close, '>', (size_t)(end - close));
				if (!after_end) return 0;
				piece->after = slice(close, after_end + 1);
				p = plain = after_end + 1;
			}
		} else if (*p == '<') {
			const char* element_end = skip_element(p, end);
			piece_t* piece;
			if (!element_end || has_interface) return 0;
			if (p != plain) add_piece(PIECE_TEXT)->before = slice(plain, p);
			piece = add_piece(PIECE_INTERFACE);
			piece->body = slice(p, element_end);
			piece->body_line = line_of(text, p);
			has_interface = 1;
			p = plain = element_end;
		} else {
			p++;
		}
	}
	if (p != plain || g_x.count == 0) add_piece(PIECE_TEXT)->before = slice(plain, end);
	return 1;
}

static void rejoin(ide_buffer_t* out) {
	int i;
	for (i = 0; i < g_x.count; i++) {
		piece_t* piece = &g_x.pieces[i];
		if (piece->kind == PIECE_TEXT) {
			ide_buffer_add(out, piece->before, strlen(piece->before));
		} else if (piece->kind == PIECE_SCRIPT) {
			const char* code = piece->editor != R_INVALID_ID && Rwidget_is_valid(piece->editor) ? Rtext_editor_get_text(piece->editor) : piece->body;
			ide_buffer_add(out, piece->before, strlen(piece->before));
			ide_buffer_add(out, code, strlen(code));
			ide_buffer_add(out, piece->after, strlen(piece->after));
		} else {
			const char* ui = designer_get_interface_text();
			size_t length = strlen(ui);
			/* the XML panel ends with a line break; the file has its own text after the element */
			while (length && (ui[length - 1] == '\n' || ui[length - 1] == '\r')) length--;
			ide_buffer_add(out, ui, length);
		}
	}
}

static int save_from_editor(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	return xmlc_save();
}

int xmlc_is_open(void) {
	return g_x.path != NULL;
}

void xmlc_close(void) {
	int i;
	for (i = 0; i < g_x.count; i++) {
		if (g_x.pieces[i].editor != R_INVALID_ID && Rwidget_is_valid(g_x.pieces[i].editor)) {
			Rdock_area_close_panel(g_ide_dock, g_x.pieces[i].editor);
		}
	}
	free_pieces();
	free(g_x.path);
	g_x.path = NULL;
}

int xmlc_open(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	int i, script = 0;
	const char* interface_text = NULL;
	if (!text) {
		ide_status("Cannot read %s", path);
		return R_FALSE;
	}
	if (xmlc_is_open()) xmlc_close();
	if (!parse(text)) {
		Rmemory_free(text);
		ide_status("%s: its elements are not closed properly; opened as text.", ide_basename(path));
		editors_open(path, 0);
		return R_FALSE;
	}
	Rmemory_free(text);
	g_x.path = ide_strdup(path);
	for (i = 0; i < g_x.count; i++) {
		piece_t* piece = &g_x.pieces[i];
		if (piece->kind == PIECE_INTERFACE) {
			interface_text = piece->body;
		} else if (piece->kind == PIECE_SCRIPT) {
			char title[300], key[1100];
			script++;
			snprintf(title, sizeof(title), "%s · script %d", ide_basename(path), script);
			snprintf(key, sizeof(key), "%s#script%d", path, script);
			piece->editor = editors_open_virtual(title, key, piece->body, ide_interface_language(path) ? ide_interface_language(path)->editor : "C",
				save_from_editor, NULL);
		}
	}
	/* a file with only scripts gets an interface to start from */
	if (!interface_text) {
		piece_t* piece = add_piece(PIECE_INTERFACE);
		piece->body = ide_strdup("<Window Title=\"Window\" Width=\"480\" Height=\"320\">\n\t<VBox/>\n</Window>");
		add_piece(PIECE_TEXT)->before = ide_strdup("\n");
		interface_text = piece->body;
	}
	designer_load_interface(interface_text, path, R_TRUE);
	return R_TRUE;
}

int xmlc_save(void) {
	ide_buffer_t out = { 0 };
	int i, ok;
	if (!g_x.path) return R_FALSE;
	rejoin(&out);
	ok = Rfile_write(g_x.path, out.data ? out.data : "", out.size);
	if (ok) {
		/* the lines of the pieces in the file, for the errors of the compiler */
		Rid editors[64];
		int count = 0;
		for (i = 0; i < g_x.count && count < 64; i++) if (g_x.pieces[i].kind == PIECE_SCRIPT) editors[count++] = g_x.pieces[i].editor;
		parse(out.data ? out.data : "");
		count = 0;
		for (i = 0; i < g_x.count; i++) {
			if (g_x.pieces[i].kind != PIECE_SCRIPT) continue;
			g_x.pieces[i].editor = count < 64 ? editors[count] : R_INVALID_ID;
			count++;
			if (g_x.pieces[i].editor != R_INVALID_ID) editors_set_modified(g_x.pieces[i].editor, R_FALSE);
		}
		designer_set_modified(R_FALSE);
		ide_status("Saved %s", g_x.path);
	} else {
		ide_status("Cannot write %s", g_x.path);
	}
	ide_buffer_free(&out);
	return ok;
}

int xmlc_interface_line(void) {
	int i;
	for (i = 0; i < g_x.count; i++) if (g_x.pieces[i].kind == PIECE_INTERFACE) return g_x.pieces[i].body_line;
	return 1;
}

int xmlc_locate(int file_line, Rid* editor, int* editor_line) {
	int i, best = -1;
	for (i = 0; i < g_x.count; i++) {
		if (g_x.pieces[i].kind == PIECE_TEXT) continue;
		if (g_x.pieces[i].body_line <= file_line) best = i;
	}
	if (best < 0) return R_FALSE;
	*editor = g_x.pieces[best].kind == PIECE_SCRIPT ? g_x.pieces[best].editor : R_INVALID_ID;
	*editor_line = file_line - g_x.pieces[best].body_line + 1;
	return R_TRUE;
}

const char* xmlc_path(void);
const char* xmlc_path(void) {
	return g_x.path;
}
