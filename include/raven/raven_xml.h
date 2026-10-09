/**
 * Raven
 * raven_xml.h : building windows and widget trees from XML.
 *
 *     <Window Title="Editor" Width="1280" Height="720">
 *       <VBox>
 *         <Label Text="Hello" Slot.Padding="8" Slot.HAlign="Center"/>
 *         <HBox Slot.Fill="1" Spacing="4">
 *           <Button Name="save" Text="Save" OnClicked="file.save"/>
 *           <Viewport Name="scene" Slot.Fill="1"/>
 *         </HBox>
 *       </VBox>
 *     </Window>
 *
 *   - An element is a widget; its tag is the widget type (VBox, Button, ...).
 *   - Child elements are child widgets, in order.
 *   - Attributes set properties. Attributes starting with "Slot." set the
 *     slot of the widget inside its parent (same meaning as Rslot_set_...).
 *   - Tags and attribute names are not case sensitive.
 *   - Name="..." gives the widget a name for Rwidget_find.
 *   - OnClicked / OnChanged / OnCommitted / OnActivated="name" set the action
 *     of the widget (Raction_bind).
 *
 * The complete list of tags and attributes is in docs/XML.md.
 */

#ifndef RAVEN_XML_H
#define RAVEN_XML_H

#include "raven_core.h"

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Creates a window from a document whose root element is <Window>.
	 * @param xml  UTF-8 text. It does not need to be null-terminated.
	 * @param size Size of the text in bytes.
	 * @return The window, or R_INVALID_ID. Parse errors are reported with
	 *         their line and column through the error callback (R_ERROR_XML).
	 */
	RAVEN_API Rid Rwindow_create_from_xml(const char* xml, size_t size);
	RAVEN_API Rid Rwindow_create_from_xml_file(const char* path);

	/**
	 * @brief Creates a widget tree from a document whose root element is a
	 * widget. The caller owns the returned widget until it is added to a parent.
	 */
	RAVEN_API Rid Rwidget_create_from_xml(const char* xml, size_t size);
	RAVEN_API Rid Rwidget_create_from_xml_file(const char* path);

	/**
	 * @brief Folder used to resolve relative paths found in XML (Source="...",
	 * Font="..."). The *FromXmlFile functions use the folder of the file.
	 */
	RAVEN_API void Rxml_set_base_path(const char* path);

	/**
	 * @brief Sets a property from text, with the names and value syntax of the
	 * XML attributes. Handy for scripting and data-driven tools.
	 * @return R_TRUE when the attribute exists for this widget and the value is valid.
	 */
	RAVEN_API int Rwidget_set_attribute(Rid widget, const char* name, const char* value);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_XML_H */
