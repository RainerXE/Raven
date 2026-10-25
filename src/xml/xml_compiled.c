/**
 * Raven - xml/xml_compiled.c
 * Interfaces compiled by ravenc: a table of elements becomes the same tree
 * the parser would make, then goes through the same expansion and building
 * as an interpreted document. No XML is read at run time.
 */

#include "xml.h"
#include "../core/context.h"

static int rvn_xml_from_compiled(rvn_xml_document_t* document, const RXmlCompiledNode* nodes, int count, const char* source, const char* function) {
	rvn_xml_node_t** made;
	int i, j;

	memset(document, 0, sizeof(*document));
	rvn_arena_init(&document->arena, 16384);
	if (!nodes || count <= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: empty table (%s)", function, source ? source : "?");
		return R_FALSE;
	}
	made = (rvn_xml_node_t**)rvn_arena_alloc(&document->arena, sizeof(rvn_xml_node_t*) * (size_t)count);
	for (i = 0; i < count; i++) {
		const RXmlCompiledNode* in = &nodes[i];
		rvn_xml_node_t* node = (rvn_xml_node_t*)rvn_arena_alloc(&document->arena, sizeof(rvn_xml_node_t));
		rvn_xml_attribute_t* last = NULL;
		memset(node, 0, sizeof(*node));
		if (!in->tag || (in->parent >= i) || (i > 0 && in->parent < 0) || (i == 0 && in->parent >= 0)) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: damaged table at element %d (%s)", function, i, source ? source : "?");
			rvn_xml_free(document);
			return R_FALSE;
		}
		/* the strings are the program's: they live as long as it does */
		node->name = in->tag;
		node->text = in->text ? in->text : "";
		node->line = in->line;
		node->column = 1;
		for (j = 0; j < in->attribute_count; j++) {
			rvn_xml_attribute_t* attribute = (rvn_xml_attribute_t*)rvn_arena_alloc(&document->arena, sizeof(rvn_xml_attribute_t));
			attribute->name = in->attributes[j * 2];
			attribute->value = in->attributes[j * 2 + 1];
			attribute->next = NULL;
			if (last) last->next = attribute;
			else node->attributes = attribute;
			last = attribute;
		}
		if (in->parent >= 0) {
			rvn_xml_node_t* parent = made[in->parent];
			node->parent = parent;
			if (parent->last_child) parent->last_child->next = node;
			else parent->first_child = node;
			parent->last_child = node;
		}
		made[i] = node;
	}
	document->root = made[0];
	if (!rvn_xml_expand(document, NULL)) {
		rvn_xml_free(document);
		return R_FALSE;
	}
	if (!document->root) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: %s only declares widgets", function, source ? source : "?");
		rvn_xml_free(document);
		return R_FALSE;
	}
	return R_TRUE;
}

Rid Rwindow_create_from_compiled(const RXmlCompiledNode* nodes, int count, const char* source) {
	rvn_xml_document_t document;
	RVN_API_GUARD(R_INVALID_ID);
	if (!rvn_xml_from_compiled(&document, nodes, count, source, __func__)) return R_INVALID_ID;
	return rvn_xml_build_window(&document, __func__);
}

Rid Rwidget_create_from_compiled(const RXmlCompiledNode* nodes, int count, const char* source) {
	rvn_xml_document_t document;
	RVN_API_GUARD(R_INVALID_ID);
	if (!rvn_xml_from_compiled(&document, nodes, count, source, __func__)) return R_INVALID_ID;
	return rvn_xml_build_widget(&document, R_FALSE, __func__);
}
