# Raven XML reference

A window, or any part of one, can be described in XML and loaded with

```c
Rid window = Rwindow_create_from_xml_file("editor.xml");   /* <Window> root  */
Rid panel  = Rwidget_create_from_xml(text, strlen(text)); /* any widget root */
```

The XML is only a way to *build* the tree: once loaded, the widgets are
ordinary handles. Find them with `Rwidget_find(window, "name")` and use the
C API on them. Anything the XML can express can also be done from C, and one
attribute can be set at run time with `Rwidget_set_attribute(widget, "Text", "Hello")`.

```xml
<Window Title="Tool" Width="800" Height="500">
    <Shortcut Keys="Ctrl+S" Action="file.save"/>
    <VBox Spacing="8" Slot.Padding="12">
        <Label TextSize="18">Settings</Label>
        <HBox Spacing="8">
            <TextInput Name="path" Placeholder="Folder..." Slot.Fill="1"/>
            <Button Text="Save" Style="Primary" OnClicked="file.save"/>
        </HBox>
        <ListView Name="files" Slot.Fill="1"/>
    </VBox>
</Window>
```

## Rules

- Tags are widget types. Tag and attribute names are not case sensitive.
- The parser handles elements, attributes, text, comments, CDATA, the five
  predefined entities (`&amp; &lt; &gt; &quot; &apos;`) and numeric character
  references. Files are UTF-8. There is no DTD, namespace or include.
- The text inside an element is its `Text` attribute: `<Label>Hello</Label>`
  is `<Label Text="Hello"/>`.
- Children are created first, then the attributes of the parent are applied,
  so `Selected="1"` or `ActiveTab="2"` can refer to children.
- An unknown tag or attribute, or an invalid value, raises a warning through
  the error callback (with line and column) and loading continues.
- Paths (`Source`, `Font`) are relative to the XML file, or to
  `Rxml_set_base_path` when loading from memory. A file is loaded once and
  shared by every widget that names it.

## Value types

| Type | Syntax |
|---|---|
| number | `12`, `0.5`, `-3` (always with a dot, whatever the locale) |
| boolean | `true` / `false`, `yes` / `no`, `1` / `0` |
| color | `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`, or a name: `transparent white black red green blue yellow orange cyan magenta gray` |
| margin | `8` (all sides), `8,4` (horizontal, vertical), `1,2,3,4` (left, top, right, bottom) |
| shortcut | `Ctrl+S`, `Ctrl+Shift+Z`, `F5`, `Alt+Enter`. Modifiers: `Ctrl`, `Shift`, `Alt`, `Super`. Keys: a letter, a digit, `F1`..`F12`, `Space Escape Enter Tab Backspace Insert Delete Left Right Up Down PageUp PageDown Home End` |

Lengths are in logical units: they are multiplied by the scale of the monitor.

## `<Window>`

Only as the root, with `Rwindow_create_from_xml[File]`. It has one child
widget, the content, and any number of `<Shortcut>`.

| Attribute | Value |
|---|---|
| `Title` | text |
| `Width`, `Height` | client size (default 800 x 600) |
| `MinWidth`, `MinHeight` | smallest size the user can give the window |
| `Resizable` | boolean (default true) |
| `Borderless`, `Maximized`, `Hidden` | boolean |
| `ClearColor` | color behind the content (default: the theme color `Window`) |
| `Background` | path of an image drawn behind the content, scaled to cover the window. With a glass theme it replaces the wallpaper of the theme |

`<Shortcut Keys="Ctrl+S" Action="file.save"/>` fires an action when the keys
are pressed anywhere in the window.

## Attributes of every widget

| Attribute | Value |
|---|---|
| `Name` (or `Id`) | name for `Rwidget_find` |
| `Width`, `Height` | fixed size, overrides the size the widget asks for |
| `MinWidth`, `MinHeight`, `MaxWidth`, `MaxHeight` | limits of the size the widget asks for |
| `Visibility` | `Visible`, `Hidden` (keeps its place), `Collapsed` (takes no place) |
| `Enabled` | boolean; a disabled widget disables its children |
| `Opacity` | 0..1, applies to the children too |
| `Clip` | boolean: children are clipped to the bounds of the widget |
| `HitTestable` | boolean: false lets the mouse through to what is behind |
| `Focusable` | boolean |
| `Tooltip` | text shown when the cursor rests on the widget |
| `Cursor` | `Arrow IBeam Hand Crosshair ResizeH ResizeV ResizeNWSE ResizeNESW Move NotAllowed` |
| `Action`, `OnClicked`, `OnChanged`, `OnCommitted`, `OnActivated` | name of the action fired by the main event of the widget (see below) |
| `Color.<Role>` | overrides a theme color for this widget, e.g. `Color.Accent="#E5484D"` |
| `Metric.<Name>` | overrides a theme metric for this widget, e.g. `Metric.CornerRadius="0"` |

Color roles: `Window Panel Border Text TextDisabled TextPlaceholder Accent
AccentHovered AccentPressed AccentText Control ControlHovered ControlPressed
ControlBorder Input Focus Selection RowHovered Track Handle HandleHovered
Separator Header HeaderHovered TabActive Popup PopupBorder Shadow Tooltip
TooltipText Switch`. `Color.Text` is inherited by the children.

Metrics: `FontSize CornerRadius BorderWidth ControlHeight PaddingX PaddingY
Spacing ScrollbarSize RowHeight Indent SplitterSize CheckSize SliderHandleSize
PopupRadius`.

With a glass theme (`Rtheme_apply(R_THEME_GLASS_DARK)` or
`R_THEME_GLASS_LIGHT`, see the README) the same tags and attributes describe
glass controls. The alpha of a color is then the strength of the tint of the
glass: `Color.Control="#FF375F60"` is a pink glass button, `#FF375F` an opaque
one.

### Actions

A widget has one action name; the five `On...` spellings are synonyms kept
for readability. The action fires on the main event of the widget:

| Widget | Fires when |
|---|---|
| Button, MenuItem | clicked |
| Checkbox, Slider, SpinBox, ComboBox, TabView, Expander | the value changes |
| TextInput | Enter is pressed |
| ListView, TreeView | a row is activated (double click or Enter) |

```c
static void on_save(Rid sender, const char* action, void* user_data) { ... }
Raction_bind("file.save", on_save, NULL);
/* or one function for everything that has no binding of its own */
Raction_set_default_handler(on_any_action, NULL);
```

Callbacks set from C work alongside actions and cover the other events:
every keystroke of a text input (`Rtext_input_set_changed_callback`), the
selection of a list or tree (`Rlist_view_set_selection_changed_callback`,
`Rtree_view_set_selection_changed_callback`), raw input (`Rwidget_set_event_callback`).

## Slot attributes

A slot belongs to the child and is read by its parent panel (Slate). Each
panel uses the attributes that make sense for it.

| Attribute | Value | Used by |
|---|---|---|
| `Slot.Padding` | margin around the child | every panel, and the window for its content |
| `Slot.HAlign` | `Fill` (default), `Left`, `Center`, `Right` | VBox, Grid, Overlay, Border, SizeBox, ScrollBox, Switcher, window content |
| `Slot.VAlign` | `Fill` (default), `Top`, `Center`, `Bottom` | HBox, Grid, Overlay, Border, SizeBox, ScrollBox, Switcher, window content |
| `Slot.Fill` | number: share of the free space along the panel (`1` = one share). Without it the child gets the size it asks for | VBox, HBox; pane size in Splitter |
| `Slot.Auto` | `true`: back to the size the child asks for | VBox, HBox |
| `Slot.MaxSize` | limit of a filling child along the panel | VBox, HBox |
| `Slot.Column`, `Slot.Row` | cell, from 0 | Grid |
| `Slot.ColumnSpan`, `Slot.RowSpan` | number of cells covered (default 1) | Grid |
| `Slot.Anchors` | `x,y` pins to a point of the parent, `minX,minY,maxX,maxY` stretches between two; fractions 0..1 | Canvas |
| `Slot.Position` | `x,y` offset from the anchor | Canvas |
| `Slot.Size` | `width,height` (when the anchors do not stretch) | Canvas |
| `Slot.Alignment` | `x,y` pivot of the child, 0..1 (`0.5,0.5` centers it on its position) | Canvas |
| `Slot.Title` | text of the tab | TabView |

## Panels

| Tag (aliases) | Children | Attributes |
|---|---|---|
| `VBox` (`VerticalBox`) | stacked top to bottom | `Spacing` |
| `HBox` (`HorizontalBox`) | left to right | `Spacing` |
| `Grid` (`GridPanel`) | placed by `Slot.Column` / `Slot.Row` | `Spacing`; `Columns`, `Rows`: one entry per track, `auto` or a fill share, e.g. `Columns="auto,1,2"` |
| `UniformGrid` (`UniformGridPanel`) | cells of equal size, filled row by row | `Columns` (children fill rows of this many cells; 0 = each child goes to its `Slot.Column` / `Slot.Row`), `MinCellSize` (`size` or `width,height`), `Spacing` |
| `Overlay` | stacked on top of each other, last on top | |
| `Border` | one child, with a background | `Padding`, `Background`, `BorderColor`, `BorderWidth`, `CornerRadius` (`r` or `topLeft,topRight,bottomRight,bottomLeft`), `Shadow` (`blur` or `blur,offsetX,offsetY`), `Material` (`Solid`, or `Glass`: a panel that blurs and refracts what is behind it, tinted by `Background` when given) |
| `SizeBox` (`Box`) | one child with a forced size (`Width`, `Height`...) | `AspectRatio` (width / height) |
| `Spacer` | none: empty space | `Width`, `Height`, or `Slot.Fill` |
| `ScrollBox` | scrolled content | `Orientation` (`Vertical` default), `ScrollBoth`, `Spacing` |
| `Splitter` | panes separated by draggable handles | `Orientation` (`Horizontal` = side by side, default), `MinPaneSize` |
| `WrapBox` | left to right, wrapping to the next line | `Spacing` |
| `Canvas` (`CanvasPanel`) | free placement with anchors | |
| `Switcher` (`WidgetSwitcher`) | shows one child at a time | `Index` |
| `TabView` (`Tabs`) | one tab per child (`Slot.Title`) | `ActiveTab` |
| `Expander` (`ExpandableArea`) | one child under a collapsible header | `Title`, `Expanded` |

## Controls

| Tag (aliases) | Attributes |
|---|---|
| `Label` (`Text`, `TextBlock`) | `Text`, `TextSize` (or `FontSize`), `Font` (path of a .ttf), `TextAlign` (`Left Center Right`), `Wrap`, `Color` |
| `Button` | `Text`, `Style` (`Default`, `Primary`, `Flat`) |
| `Checkbox` (`CheckBox`) | `Text`, `Checked`, `Style` (`Check`, `Radio`, `Switch`). Radio boxes of the same parent are exclusive |
| `Slider` | `Min`, `Max`, `Value`, `Step`, `Orientation` |
| `ProgressBar` | `Value` (0..1) |
| `SpinBox` | `Min`, `Max`, `Value`, `Step`, `Decimals` |
| `TextInput` (`TextBox`, `EditableText`) | `Text`, `Placeholder`, `Password`, `ReadOnly`, `MaxLength` |
| `ComboBox` | `Selected` (index), items as children `<Item Text="..."/>` or `Items="A|B|C"` |
| `ListView` | `Selected`, items as children `<Item>` or `Items="A|B|C"` (large lists: `Rlist_view_set_virtual_items`) |
| `TreeView` | nested `<Item Text="..." Expanded="true">` children |
| `Image` | `Source` (path of an image), `Tint`, `Stretch` (`Fill Fit Cover None`), `Region` (`u0,v0,u1,v1`), `CornerRadius` |
| `Separator` | `Orientation` (`Horizontal` line by default) |
| `MenuBar` | `<Menu>` children |
| `Menu` | `Text`; children `<MenuItem>`, `<Separator/>` and nested `<Menu>` |
| `MenuItem` | `Text`, `Shortcut`, `Checkable`, `Checked` |
| `Viewport` | `Source` (image shown until the application sets a texture), `Stretch`, `FlipY`, `Realtime` |
| `Custom` | drawn by the application: `Rcustom_set_paint_callback` |

A `MenuItem` shortcut works without any `<Shortcut>` element: the window
looks for it in its menus when the keys are pressed.

## A complete example

`examples/editor/editor.xml` is a full editor layout: menu bar, tool bar,
nested splitters, tree, tabs with a viewport and a custom widget, a property
grid and a log.
