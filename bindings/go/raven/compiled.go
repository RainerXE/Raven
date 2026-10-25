package raven

// Interfaces compiled by ravenc (.xmlgo files). Written by hand, not by bindgen.
//
//	//go:generate ravenc main.xmlgo -o main.xmlgo.go
//
// The generated file calls CreateCompiled with the elements of the interface:
// no XML is read when the program runs.

/*
#include <stdlib.h>
#include <Raven.h>
*/
import "C"

import "unsafe"

// CompiledNode is an element of an interface compiled by ravenc.
type CompiledNode struct {
	Tag        string
	Text       string
	Attributes []string // name, value, name, value...
	Parent     int      // index of the parent, -1 for the root
	Line       int      // in the .xmlgo, for the errors
}

// CreateCompiled builds a window (window = true) or a widget tree from compiled elements.
func CreateCompiled(nodes []CompiledNode, window bool, source string) ID {
	count := len(nodes)
	if count == 0 {
		return R_INVALID_ID
	}
	var allocated []unsafe.Pointer
	text := func(s string) *C.char {
		p := C.CString(s)
		allocated = append(allocated, unsafe.Pointer(p))
		return p
	}
	// (C memory: what C is given must not hold Go pointers)
	tableMemory := C.malloc(C.size_t(count) * C.size_t(unsafe.Sizeof(C.RXmlCompiledNode{})))
	allocated = append(allocated, tableMemory)
	table := unsafe.Slice((*C.RXmlCompiledNode)(tableMemory), count)
	for i, node := range nodes {
		table[i].tag = text(node.Tag)
		table[i].text = text(node.Text)
		table[i].attributes = nil
		table[i].attribute_count = C.int(len(node.Attributes) / 2)
		if len(node.Attributes) > 0 {
			memory := C.malloc(C.size_t(len(node.Attributes)) * C.size_t(unsafe.Sizeof((*C.char)(nil))))
			allocated = append(allocated, memory)
			attributes := unsafe.Slice((**C.char)(memory), len(node.Attributes))
			for k, attribute := range node.Attributes {
				attributes[k] = text(attribute)
			}
			table[i].attributes = (**C.char)(memory)
		}
		table[i].parent = C.int(node.Parent)
		table[i].line = C.int(node.Line)
	}
	name := text(source)
	var root C.Rid
	if window {
		root = C.Rwindow_create_from_compiled(&table[0], C.int(count), name)
	} else {
		root = C.Rwidget_create_from_compiled(&table[0], C.int(count), name)
	}
	for _, p := range allocated {
		C.free(p)
	}
	return ID(root)
}
