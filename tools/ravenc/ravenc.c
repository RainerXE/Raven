/**
 * Raven - tools/ravenc/ravenc.c
 * The XMLC compiler: an interface in XML with its C code, in one file, turned
 * into one C source. Nothing is read or interpreted at run time.
 *
 *     ravenc main.xmlc -o main.xmlc.c [--name main]
 *
 * An .xmlc file:
 *
 *     <script>
 *     #include <stdio.h>
 *     static int clicks = 0;
 *     static void on_ok(Rid sender, const char* action, void* user) {
 *         Rlabel_set_text(ui.status, "Clicked");      // ui: the widgets that have a Name
 *         clicks++;
 *     }
 *     int main(void) {
 *         Rinit();
 *         main_create();                              // made by ravenc from the file name
 *         while (Rdraw_frame()) {}
 *         Rshutdown();
 *         return 0;
 *     }
 *     </script>
 *
 *     <Window Title="Hello" Width="400" Height="200">
 *         <VBox>
 *             <Label Name="status">Not clicked</Label>
 *             <Button Text="OK" OnClicked="{on_ok}"/>
 *         </VBox>
 *     </Window>
 *
 * - <script> holds plain C (no XML escaping: a < b and && are written as is),
 *   at the top level of the file, as many as wanted, before or after the
 *   interface. They are copied in order, with #line, so the compiler errors
 *   point into the .xmlc.
 * - One interface element at the top level: a <Window> (NAME_create returns
 *   the window) or any widget (NAME_create returns the widget).
 * - Action="{f}", OnClicked="{f}", OnChanged="{f}", OnCommitted="{f}",
 *   OnActivated="{f}": the C function f, an RActionCallback
 *   void f(Rid sender, const char* action, void* user_data).
 *   It is checked by the C compiler. Without braces, an action name as in XML.
 * - Name="x" (a C identifier): ui.x is that widget once NAME_create has run.
 * - Everything else is the XML of Raven, built exactly as Rwindow_create_from_xml would.
 *
 * The generated file declares, before the scripts:
 *     static struct { Rid x; ... } ui;
 *     Rid NAME_create(void);
 * No header is written: another C file declares Rid NAME_create(void); itself.
 *
 * Standalone (C99, no dependency): it runs on the build machine.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 *  Small tools
 * ------------------------------------------------------------------------- */

static const char* g_path = "";

static void die(int line, const char* format, ...) {
	va_list args;
	fprintf(stderr, "%s:%d: error: ", g_path, line);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);
	exit(1);
}

static void* xalloc(size_t size) {
	void* p = calloc(1, size ? size : 1);
	if (!p) { fprintf(stderr, "ravenc: out of memory\n"); exit(1); }
	return p;
}

static char* xstrndup(const char* s, size_t n) {
	char* p = (char*)xalloc(n + 1);
	memcpy(p, s, n);
	p[n] = 0;
	return p;
}

typedef struct buffer_t {
	char* data;
	size_t size, cap;
	int lines;
} buffer_t;

static void buf_add(buffer_t* b, const char* s, size_t n) {
	size_t i;
	if (b->size + n + 1 > b->cap) {
		b->cap = (b->size + n + 1) * 2;
		b->data = (char*)realloc(b->data, b->cap);
		if (!b->data) { fprintf(stderr, "ravenc: out of memory\n"); exit(1); }
	}
	memcpy(b->data + b->size, s, n);
	b->size += n;
	b->data[b->size] = 0;
	for (i = 0; i < n; i++) if (s[i] == '\n') b->lines++;
}

static void buf_puts(buffer_t* b, const char* s) { buf_add(b, s, strlen(s)); }

static void buf_printf(buffer_t* b, const char* format, ...) {
	char small[1024];
	va_list args;
	int n;
	va_start(args, format);
	n = vsnprintf(small, sizeof(small), format, args);
	va_end(args);
	if (n < 0) return;
	if ((size_t)n < sizeof(small)) { buf_add(b, small, (size_t)n); return; }
	{
		char* big = (char*)xalloc((size_t)n + 1);
		va_start(args, format);
		vsnprintf(big, (size_t)n + 1, format, args);
		va_end(args);
		buf_add(b, big, (size_t)n);
		free(big);
	}
}

/* A C string literal of any text (UTF-8 kept as is). */
static void buf_cstring(buffer_t* b, const char* s) {
	buf_puts(b, "\"");
	for (; *s; s++) {
		unsigned char c = (unsigned char)*s;
		if (c == '"' || c == '\\') { char e[3] = { '\\', (char)c, 0 }; buf_puts(b, e); }
		else if (c == '\n') buf_puts(b, "\\n");
		else if (c == '\r') buf_puts(b, "\\r");
		else if (c == '\t') buf_puts(b, "\\t");
		else if (c == '?' && s[1] == '?') buf_puts(b, "?\"\"");  /* no trigraphs */
		else if (c < 0x20) buf_printf(b, "\\%03o", c);
		else { char e[2] = { (char)c, 0 }; buf_puts(b, e); }
	}
	buf_puts(b, "\"");
}

static int is_identifier(const char* s) {
	if (!s || !(isalpha((unsigned char)*s) || *s == '_')) return 0;
	for (s++; *s; s++) if (!(isalnum((unsigned char)*s) || *s == '_')) return 0;
	return 1;
}

static int iequal(const char* a, const char* b) {
	for (; *a && *b; a++, b++) if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
	return *a == *b;
}

/* ---------------------------------------------------------------------------
 *  The file: scripts and elements
 * ------------------------------------------------------------------------- */

typedef struct attribute_t {
	char* name;
	char* value;
	struct attribute_t* next;
} attribute_t;

typedef struct node_t {
	char* tag;
	char* text;           /* text content, trimmed */
	attribute_t* attributes;
	int attribute_count;
	struct node_t* parent;
	struct node_t* first_child;
	struct node_t* last_child;
	struct node_t* next;
	int line;
	int index;            /* in the table */
} node_t;

typedef struct script_t {
	char* code;
	int line;             /* line of the first character of the code */
} script_t;

typedef struct parser_t {
	const char* p;
	const char* end;
	int line;
	script_t* scripts;
	int script_count;
	node_t* root;
} parser_t;

static void advance(parser_t* ps, size_t n) {
	size_t i;
	for (i = 0; i < n && ps->p < ps->end; i++, ps->p++) if (*ps->p == '\n') ps->line++;
}

static int starts(parser_t* ps, const char* s) {
	size_t n = strlen(s);
	return (size_t)(ps->end - ps->p) >= n && memcmp(ps->p, s, n) == 0;
}

static int starts_i(parser_t* ps, const char* s) {
	size_t n = strlen(s), i;
	if ((size_t)(ps->end - ps->p) < n) return 0;
	for (i = 0; i < n; i++) if (tolower((unsigned char)ps->p[i]) != tolower((unsigned char)s[i])) return 0;
	return 1;
}

static void skip_space(parser_t* ps) {
	while (ps->p < ps->end && isspace((unsigned char)*ps->p)) advance(ps, 1);
}

/* Skips what is not content: comments, <?...?>, <!DOCTYPE ...>. R_TRUE when something was skipped. */
static int skip_misc(parser_t* ps) {
	if (starts(ps, "<!--")) {
		int line = ps->line;
		while (ps->p < ps->end && !starts(ps, "-->")) advance(ps, 1);
		if (ps->p >= ps->end) die(line, "unterminated comment");
		advance(ps, 3);
		return 1;
	}
	if (starts(ps, "<?") || starts(ps, "<!DOCTYPE") || starts(ps, "<!doctype")) {
		int line = ps->line;
		while (ps->p < ps->end && *ps->p != '>') advance(ps, 1);
		if (ps->p >= ps->end) die(line, "unterminated declaration");
		advance(ps, 1);
		return 1;
	}
	return 0;
}

static char* read_name(parser_t* ps) {
	const char* start = ps->p;
	while (ps->p < ps->end && (isalnum((unsigned char)*ps->p) || *ps->p == '_' || *ps->p == '-' || *ps->p == '.' || *ps->p == ':' || (unsigned char)*ps->p >= 0x80))
		advance(ps, 1);
	if (ps->p == start) die(ps->line, "a name is expected here");
	return xstrndup(start, (size_t)(ps->p - start));
}

/* Appends the UTF-8 of a code point. */
static void put_utf8(buffer_t* b, unsigned long c) {
	char s[5] = { 0 };
	if (c < 0x80) s[0] = (char)c;
	else if (c < 0x800) { s[0] = (char)(0xC0 | (c >> 6)); s[1] = (char)(0x80 | (c & 63)); }
	else if (c < 0x10000) { s[0] = (char)(0xE0 | (c >> 12)); s[1] = (char)(0x80 | ((c >> 6) & 63)); s[2] = (char)(0x80 | (c & 63)); }
	else { s[0] = (char)(0xF0 | (c >> 18)); s[1] = (char)(0x80 | ((c >> 12) & 63)); s[2] = (char)(0x80 | ((c >> 6) & 63)); s[3] = (char)(0x80 | (c & 63)); }
	buf_puts(b, s);
}

/* Reads an entity after '&'. */
static void read_entity(parser_t* ps, buffer_t* out) {
	int line = ps->line;
	const char* semi = ps->p;
	char name[16];
	size_t n;
	while (semi < ps->end && *semi != ';' && semi - ps->p < 12) semi++;
	if (semi >= ps->end || *semi != ';') die(line, "'&' must start an entity (&amp; for the character itself)");
	n = (size_t)(semi - ps->p);
	memcpy(name, ps->p, n);
	name[n] = 0;
	if (!strcmp(name, "amp")) buf_puts(out, "&");
	else if (!strcmp(name, "lt")) buf_puts(out, "<");
	else if (!strcmp(name, "gt")) buf_puts(out, ">");
	else if (!strcmp(name, "quot")) buf_puts(out, "\"");
	else if (!strcmp(name, "apos")) buf_puts(out, "'");
	else if (name[0] == '#') {
		char* tail;
		unsigned long c = (name[1] == 'x' || name[1] == 'X') ? strtoul(name + 2, &tail, 16) : strtoul(name + 1, &tail, 10);
		if (*tail || c == 0 || c > 0x10FFFF) die(line, "bad character reference &%s;", name);
		put_utf8(out, c);
	} else die(line, "unknown entity &%s;", name);
	advance(ps, n + 1);
}

static char* trim(char* s) {
	char* e;
	while (*s && isspace((unsigned char)*s)) s++;
	e = s + strlen(s);
	while (e > s && isspace((unsigned char)e[-1])) e--;
	*e = 0;
	return s;
}

static void read_script(parser_t* ps, int line) {
	/* after <script ...>: plain C until </script> */
	const char* start;
	int code_line;
	skip_space(ps);
	if (starts(ps, "/>")) { advance(ps, 2); return; }
	if (*ps->p != '>') die(ps->line, "<script> takes no attributes");
	advance(ps, 1);
	start = ps->p;
	code_line = ps->line;
	while (ps->p < ps->end && !starts_i(ps, "</script")) advance(ps, 1);
	if (ps->p >= ps->end) die(line, "<script> is not closed (</script>)");
	ps->scripts = (script_t*)realloc(ps->scripts, sizeof(script_t) * (size_t)(ps->script_count + 1));
	ps->scripts[ps->script_count].code = xstrndup(start, (size_t)(ps->p - start));
	ps->scripts[ps->script_count].line = code_line;
	ps->script_count++;
	advance(ps, 8);
	skip_space(ps);
	if (*ps->p != '>') die(ps->line, "'>' expected after </script");
	advance(ps, 1);
}

static node_t* read_element(parser_t* ps, node_t* parent);

/* Content of an element until its closing tag. */
static void read_content(parser_t* ps, node_t* node) {
	buffer_t text = { 0 };
	for (;;) {
		if (ps->p >= ps->end) die(node->line, "<%s> is not closed", node->tag);
		if (starts(ps, "</")) {
			char* name;
			advance(ps, 2);
			name = read_name(ps);
			if (strcmp(name, node->tag) != 0) die(ps->line, "</%s> closes <%s>", name, node->tag);
			free(name);
			skip_space(ps);
			if (*ps->p != '>') die(ps->line, "'>' expected");
			advance(ps, 1);
			break;
		}
		if (starts(ps, "<![CDATA[")) {
			const char* start;
			advance(ps, 9);
			start = ps->p;
			while (ps->p < ps->end && !starts(ps, "]]>")) advance(ps, 1);
			if (ps->p >= ps->end) die(node->line, "unterminated CDATA");
			buf_add(&text, start, (size_t)(ps->p - start));
			advance(ps, 3);
			continue;
		}
		if (skip_misc(ps)) continue;
		if (*ps->p == '<') {
			if (starts_i(ps, "<script")) die(ps->line, "<script> goes at the top level of the file, not inside <%s>", node->tag);
			read_element(ps, node);
			continue;
		}
		if (*ps->p == '&') { advance(ps, 1); read_entity(ps, &text); continue; }
		buf_add(&text, ps->p, 1);
		advance(ps, 1);
	}
	node->text = xstrndup(text.data ? trim(text.data) : "", text.data ? strlen(trim(text.data)) : 0);
	free(text.data);
}

static node_t* read_element(parser_t* ps, node_t* parent) {
	node_t* node = (node_t*)xalloc(sizeof(node_t));
	attribute_t* last = NULL;
	node->line = ps->line;
	advance(ps, 1); /* < */
	node->tag = read_name(ps);
	node->parent = parent;
	if (parent) {
		if (parent->last_child) parent->last_child->next = node;
		else parent->first_child = node;
		parent->last_child = node;
	}
	for (;;) {
		skip_space(ps);
		if (ps->p >= ps->end) die(node->line, "<%s> is not closed", node->tag);
		if (starts(ps, "/>")) { advance(ps, 2); node->text = xstrndup("", 0); return node; }
		if (*ps->p == '>') { advance(ps, 1); read_content(ps, node); return node; }
		{
			attribute_t* a = (attribute_t*)xalloc(sizeof(attribute_t));
			buffer_t value = { 0 };
			char quote;
			a->name = read_name(ps);
			skip_space(ps);
			if (*ps->p != '=') die(ps->line, "'=' expected after %s", a->name);
			advance(ps, 1);
			skip_space(ps);
			quote = *ps->p;
			if (quote != '"' && quote != '\'') die(ps->line, "the value of %s must be in quotes", a->name);
			advance(ps, 1);
			while (ps->p < ps->end && *ps->p != quote) {
				if (*ps->p == '&') { advance(ps, 1); read_entity(ps, &value); continue; }
				if (*ps->p == '<') die(ps->line, "'<' in a value: write &lt;");
				buf_add(&value, ps->p, 1);
				advance(ps, 1);
			}
			if (ps->p >= ps->end) die(node->line, "unterminated value of %s", a->name);
			advance(ps, 1);
			a->value = value.data ? value.data : xstrndup("", 0);
			if (last) last->next = a; else node->attributes = a;
			last = a;
			node->attribute_count++;
		}
	}
}

static void parse(parser_t* ps) {
	for (;;) {
		skip_space(ps);
		if (ps->p >= ps->end) break;
		if (skip_misc(ps)) continue;
		if (starts_i(ps, "<script")) {
			int line = ps->line;
			advance(ps, 7);
			if (ps->p < ps->end && !isspace((unsigned char)*ps->p) && *ps->p != '>' && *ps->p != '/') die(line, "unknown element at the top level");
			read_script(ps, line);
			continue;
		}
		if (*ps->p == '<') {
			int line = ps->line;
			node_t* node = read_element(ps, NULL);
			if (ps->root) die(line, "a second interface element <%s>: an .xmlc file has one (<Window> or a widget), and <script> blocks", node->tag);
			ps->root = node;
			continue;
		}
		die(ps->line, "text outside of an element (C code goes in <script>)");
	}
}

/* ---------------------------------------------------------------------------
 *  C
 * ------------------------------------------------------------------------- */

typedef struct binding_t {
	char* function;
	char* action;
	int line;
} binding_t;

static node_t** g_table;
static int g_count;
static char** g_names;
static int g_name_count;
static binding_t* g_bindings;
static int g_binding_count;

static int is_event(const char* name) {
	return iequal(name, "Action") || iequal(name, "OnClicked") || iequal(name, "OnChanged") ||
		iequal(name, "OnCommitted") || iequal(name, "OnActivated");
}

static int inside_declaration(const node_t* node) {
	/* the body of a <Widget> declaration is a template: its names belong to each use */
	for (node = node->parent; node; node = node->parent) if (iequal(node->tag, "Widget")) return 1;
	return 0;
}

static void collect(node_t* node, const char* unit) {
	attribute_t* a;
	node_t* child;
	node->index = g_count;
	g_table = (node_t**)realloc(g_table, sizeof(node_t*) * (size_t)(g_count + 1));
	g_table[g_count++] = node;
	for (a = node->attributes; a; a = a->next) {
		size_t n = strlen(a->value);
		if (is_event(a->name) && n >= 2 && a->value[0] == '{' && a->value[n - 1] == '}') {
			char* function = trim(xstrndup(a->value + 1, n - 2));
			char action[512];
			int i, found = -1;
			if (!is_identifier(function)) die(node->line, "%s=\"%s\": the braces hold the name of a C function", a->name, a->value);
			for (i = 0; i < g_binding_count; i++) if (!strcmp(g_bindings[i].function, function)) found = i;
			if (found < 0) {
				snprintf(action, sizeof(action), "ravenc.%s.%s", unit, function);
				g_bindings = (binding_t*)realloc(g_bindings, sizeof(binding_t) * (size_t)(g_binding_count + 1));
				g_bindings[g_binding_count].function = function;
				g_bindings[g_binding_count].action = xstrndup(action, strlen(action));
				g_bindings[g_binding_count].line = node->line;
				found = g_binding_count++;
			}
			a->value = g_bindings[found].action;
		}
		if (iequal(a->name, "Name") && is_identifier(a->value) && !inside_declaration(node)) {
			int i, seen = 0;
			for (i = 0; i < g_name_count; i++) if (!strcmp(g_names[i], a->value)) seen = 1;
			if (!seen) {
				g_names = (char**)realloc(g_names, sizeof(char*) * (size_t)(g_name_count + 1));
				g_names[g_name_count++] = a->value;
			}
		}
	}
	for (child = node->first_child; child; child = child->next) collect(child, unit);
}

static void line_directive(buffer_t* out, int line, const char* path) {
	buf_printf(out, "#line %d ", line);
	buf_cstring(out, path);
	buf_puts(out, "\n");
}

static char* read_file(const char* path, size_t* size) {
	FILE* f = fopen(path, "rb");
	char* data;
	long n;
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (char*)xalloc((size_t)n + 1);
	if (n > 0 && fread(data, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(data); return NULL; }
	fclose(f);
	*size = (size_t)n;
	return data;
}

static char* forward_slashes(const char* s) {
	char* r = xstrndup(s, strlen(s)), *p;
	for (p = r; *p; p++) if (*p == '\\') *p = '/';
	return r;
}

/* "dir/my-app.xmlc" -> "my_app" */
static char* unit_name(const char* path) {
	const char* base = path;
	const char* p;
	char* name;
	size_t n, i;
	for (p = path; *p; p++) if (*p == '/' || *p == '\\') base = p + 1;
	n = strcspn(base, ".");
	name = xstrndup(base, n ? n : 1);
	if (!n) name[0] = 'x';
	for (i = 0; name[i]; i++) if (!isalnum((unsigned char)name[i])) name[i] = '_';
	if (isdigit((unsigned char)name[0])) { char* r = (char*)xalloc(strlen(name) + 2); r[0] = '_'; strcpy(r + 1, name); free(name); name = r; }
	return name;
}

static int usage(void) {
	fprintf(stderr,
		"ravenc: compiles an .xmlc file (Raven XML + C) into one C file\n"
		"usage: ravenc <file.xmlc> -o <file.c> [--name NAME]\n"
		"  --name NAME   the function is NAME_create (default: the file name)\n");
	return 2;
}

int main(int argc, char** argv) {
	const char *input = NULL, *output = NULL, *name = NULL;
	parser_t ps;
	buffer_t out = { 0 };
	char* source;
	char* source_path;
	char* out_path;
	char* unit;
	size_t size = 0;
	int i, is_window;
	FILE* f;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-o") && i + 1 < argc) output = argv[++i];
		else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) return usage();
		else if (argv[i][0] == '-') return usage();
		else if (!input) input = argv[i];
		else return usage();
	}
	if (!input || !output) return usage();
	g_path = input;
	source = read_file(input, &size);
	if (!source) { fprintf(stderr, "ravenc: cannot read %s\n", input); return 1; }
	unit = name ? xstrndup(name, strlen(name)) : unit_name(input);
	if (!is_identifier(unit)) { fprintf(stderr, "ravenc: --name must be a C identifier\n"); return 2; }

	memset(&ps, 0, sizeof(ps));
	ps.p = source;
	ps.end = source + size;
	ps.line = 1;
	if (size >= 3 && (unsigned char)source[0] == 0xEF && (unsigned char)source[1] == 0xBB && (unsigned char)source[2] == 0xBF) ps.p += 3;
	parse(&ps);
	if (ps.root) collect(ps.root, unit);
	is_window = ps.root && iequal(ps.root->tag, "Window");

	source_path = forward_slashes(input);
	out_path = forward_slashes(output);

	buf_printf(&out, "/* Generated by ravenc from %s: edit the .xmlc, not this file. */\n", source_path);
	buf_puts(&out, "#include <Raven.h>\n\n");
	if (g_name_count) {
		buf_puts(&out, "/* the widgets that have a Name, filled by the create function */\n");
		buf_puts(&out, "static struct {\n");
		for (i = 0; i < g_name_count; i++) buf_printf(&out, "\tRid %s;\n", g_names[i]);
		buf_puts(&out, "} ui;\n\n");
	}
	if (ps.root) buf_printf(&out, "Rid %s_create(void);\n\n", unit);

	for (i = 0; i < ps.script_count; i++) {
		line_directive(&out, ps.scripts[i].line, source_path);
		buf_puts(&out, ps.scripts[i].code);
		buf_puts(&out, "\n");
		line_directive(&out, out.lines + 2, out_path);
	}

	if (ps.root) {
		buf_puts(&out, "\n/* ---- the interface ---- */\n\n");
		for (i = 0; i < g_count; i++) {
			node_t* n = g_table[i];
			attribute_t* a;
			if (!n->attribute_count) continue;
			buf_printf(&out, "static const char* const ravenc_a%d[] = { ", i);
			for (a = n->attributes; a; a = a->next) {
				buf_cstring(&out, a->name);
				buf_puts(&out, ", ");
				buf_cstring(&out, a->value);
				buf_puts(&out, a->next ? ", " : " ");
			}
			buf_puts(&out, "};\n");
		}
		buf_printf(&out, "\nstatic const RXmlCompiledNode ravenc_nodes[%d] = {\n", g_count);
		for (i = 0; i < g_count; i++) {
			node_t* n = g_table[i];
			buf_puts(&out, "\t{ ");
			buf_cstring(&out, n->tag);
			buf_puts(&out, ", ");
			buf_cstring(&out, n->text);
			if (n->attribute_count) buf_printf(&out, ", ravenc_a%d, %d", i, n->attribute_count);
			else buf_puts(&out, ", 0, 0");
			buf_printf(&out, ", %d, %d },\n", n->parent ? n->parent->index : -1, n->line);
		}
		buf_puts(&out, "};\n\n");

		buf_printf(&out, "Rid %s_create(void) {\n\tRid root;\n", unit);
		for (i = 0; i < g_binding_count; i++) {
			/* (a missing or mistyped function is reported on the line of its element) */
			line_directive(&out, g_bindings[i].line, source_path);
			buf_printf(&out, "\tRaction_bind(");
			buf_cstring(&out, g_bindings[i].action);
			buf_printf(&out, ", %s, NULL);\n", g_bindings[i].function);
		}
		if (g_binding_count) line_directive(&out, out.lines + 2, out_path);
		buf_printf(&out, "\troot = %s(ravenc_nodes, %d, ", is_window ? "Rwindow_create_from_compiled" : "Rwidget_create_from_compiled", g_count);
		buf_cstring(&out, source_path);
		buf_puts(&out, ");\n");
		for (i = 0; i < g_name_count; i++) {
			buf_printf(&out, "\tui.%s = root ? Rwidget_find(root, ", g_names[i]);
			buf_cstring(&out, g_names[i]);
			buf_puts(&out, ") : R_INVALID_ID;\n");
		}
		buf_puts(&out, "\treturn root;\n}\n");
	}

	f = fopen(output, "wb");
	if (!f || fwrite(out.data, 1, out.size, f) != out.size || fclose(f) != 0) {
		fprintf(stderr, "ravenc: cannot write %s\n", output);
		return 1;
	}
	return 0;
}
