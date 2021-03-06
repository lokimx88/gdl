#!/usr/bin/env seed

Gtk = imports.gi.Gtk;
Gdl = imports.gi.Gdl;

Gtk.init (Seed.argv);

function create_item (button_title) {
	vbox1 = new Gtk.VBox.c_new (false, 0);

	button1 = new Gtk.Button.with_label (button_title);
	vbox1.pack_start (button1, true, true, 0);
	vbox1.show_all ();

	return vbox1;
}

function create_text_item () {
	vbox1 = new Gtk.VBox.c_new (false, 0);
	scrolledwindow1 = new Gtk.ScrolledWindow.c_new (null, null);

	vbox1.pack_start (scrolledwindow1, true, true, 0);
	scrolledwindow1.set_policy (Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
	scrolledwindow1.set_shadow_type (Gtk.ShadowType.ETCHED_IN);

	text = new Gtk.TextView ({ "wrap-mode": Gtk.WrapMode.WORD });
	scrolledwindow1.add (text);

	vbox1.show_all ();
	return vbox1;
}

function create_styles_item (dock) {
	function create_style_button (dock, box, group, style, style_text) {
		current_style = dock.master.switcher_style;
		/* This won't work because of a bug in G-I, a workaround is below

		button1 = new Gtk.RadioButton.with_label_from_widget (group, style_text);
		*/

		if (group == null) {
			button1 = new Gtk.RadioButton ();
		} else {
			button1 = group.new_with_label_from_widget (style_text);
		}
		/* end workaround */

		button1.show ();
		button1.__style_id = style;
		if (current_style == style) {
	    		button1.set_active (true);
		}
		button1.signal.toggled.connect (function (button, dock) {
			style = button.__style_id;
			active = button.get_active ();
			if (active) {
			    dock.master.switcher_style = style;
			}
		}, dock);

		box.pack_start (button1, false, false, 0);
		return button1;
	}

	vbox1 = new Gtk.VBox.c_new (false, 0);
	vbox1.show ();

	group = create_style_button (dock, vbox1, null,
				     Gdl.SwitcherStyle.ICON, "Only icon");
	group = create_style_button (dock, vbox1, group,
				     Gdl.SwitcherStyle.TEXT, "Only text");
	group = create_style_button (dock, vbox1, group,
				     Gdl.SwitcherStyle.BOTH, "Both icons and texts");
	group = create_style_button (dock, vbox1, group,
				     Gdl.SwitcherStyle.TOOLBAR, "Desktop toolbar style");
	group = create_style_button (dock, vbox1, group,
				     Gdl.SwitcherStyle.TABS, "Notebook tabs");
	group = create_style_button (dock, vbox1, group,
				     Gdl.SwitcherStyle.NONE, "None of the above");
	return vbox1;
}


win = new Gtk.Window ({ "title": "Docking widget test", "default-width": 400, "default-height": 400 });
win.signal.delete_event.connect (function () {
	Gtk.main_quit ();
	return true;
});

table = new Gtk.VBox.c_new (false, 5);
win.add (table);

dock = new Gdl.Dock ();

layout = new Gdl.DockLayout (dock);
dockbar = new Gdl.DockBar (dock);
dockbar.docbar_style = Gdl.DockBarStyle.TEXT;

box = new Gtk.HBox.c_new (false, 5);
table.pack_start (box, true, true, 0);

box.pack_start (dockbar, false, false, 0);
box.pack_end (dock, true, true);

item1 = new Gdl.DockItem.c_new ("item1", "Item #1", Gdl.DockItemBehavior.LOCKED);
item1.add (create_text_item ());

dock.add_item (item1, Gdl.DockPlacement.TOP);
item1.show ();

item2 = new Gdl.DockItem.with_stock ("item2", "Item #2: Select the switcher style for notebooks",
					Gtk.STOCK_EXECUTE, Gdl.DockItemBehavior.NORMAL);
item2.resize = false;
item2.add (create_styles_item (dock));

dock.add_item (item2, Gdl.DockPlacement.RIGHT);
item2.show_all ();

item3 = new Gdl.DockItem.with_stock ("item3", "Item #3 has accented characters (áéíóúñ)",
				Gtk.STOCK_CONVERT, Gdl.DockItemBehavior.NORMAL |
					      Gdl.DockItemBehavior.CANT_CLOSE);
item3.add (create_item ("Button 3"));
dock.add_item (item3, Gdl.DockPlacement.BOTTOM);
item3.show ();

items = new Array (3);
items [0] = new Gdl.DockItem.with_stock ("Item #4", "Item #4", Gtk.STOCK_JUSTIFY_FILL,
						  Gdl.DockItemBehavior.NORMAL |
						  Gdl.DockItemBehavior.CANT_ICONIFY);
items [0].add (create_text_item ());
items [0].show ();
dock.add_item (items [0], Gdl.DockPlacement.BOTTOM);
for (i = 1; i < 3; i++) {
	name = "Item #" + (i + 4);
	items [i] = new Gdl.DockItem.with_stock (name, name, Gtk.STOCK_NEW,
						      Gdl.DockItemBehavior.NORMAL);
	items [i].add (create_text_item ());
	items [i].show ();

	items [0].dock (items [i], Gdl.DockPlacement.CENTER, null);
};

item3.dock_to (item1, Gdl.DockPlacement.TOP, -1);

item2.dock_to (item3, Gdl.DockPlacement.RIGHT, -1);

item2.dock_to (item3, Gdl.DockPlacement.LEFT, -1);

item2.dock_to (null, Gdl.DockPlacement.FLOATING, -1);

box = new Gtk.HBox.c_new (true, 5);
table.pack_end (box, false, false, 0);

button = new Gtk.Button.from_stock (Gtk.STOCK_SAVE);
button.signal.clicked.connect (function (w, layout) {
	dialog = new Gtk.Dialog ({title: "New Layout"});
	dialog.flags = Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT;
	dialog.add_button (Gtk.STOCK_OK, Gtk.ResponseType.OK);

	hbox = new Gtk.HBox.c_new (false, 8);
	hbox.set_border_width (8);
	dialog.get_content_area ().pack_start (hbox, false, false, 0);

	label = new Gtk.Label.c_new ("Name:");
	hbox.pack_start (label, false, false, 0);
	
	entry = new Gtk.Entry.c_new ();
	hbox.pack_start (entry, true, true, 0);

	hbox.show_all ();
	response = dialog.run ();

	if (response == Gtk.ResponseType.OK) {
		name = entry.get_text ();
		layout.save_layout (name);
	}
	dialog.destroy ();
}, layout);

box.pack_end (button, false, true, 0);

button = new Gtk.Button.with_label ("Layout Manager");
button.signal.clicked.connect (function (w, layout) {
	layout.run_manager (layout);
}, layout);
box.pack_end (button, false, true, 0);
	
button = new Gtk.Button.with_label ("Dump XML");
button.signal.clicked.connect (function (w, layout) {
	layout.save_to_file ("layout.xml");
} , layout);
box.pack_end (button, false, true, 0);
	
win.show_all ();

new Gdl.DockPlaceholder.c_new ("ph1", dock, Gdl.DockPlacement.TOP, false);
new Gdl.DockPlaceholder.c_new ("ph2", dock, Gdl.DockPlacement.BOTTOM, false);
new Gdl.DockPlaceholder.c_new ("ph3", dock, Gdl.DockPlacement.LEFT, false);
new Gdl.DockPlaceholder.c_new ("ph4", dock, Gdl.DockPlacement.RIGHT, false);

Gtk.main ();
