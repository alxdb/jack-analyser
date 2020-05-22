extern crate gtk;
extern crate gio;

use gtk::prelude::*;
use gio::prelude::*;
use gtk::{Application, ApplicationWindow, Button};

fn main() {
    let application = gtk::Application::new(Some("adb.jackscope"), Default::default()).expect("failed to init GTK application");
    application.connect_activate(|app| {
        let window = ApplicationWindow::new(app);
        window.set_title("First GTK+ Program");
        window.set_default_size(350, 70);

        let button = Button::new_with_label("Click me!");
        button.connect_clicked(|_| {
            println!("Clicked!");
        });
        window.add(&button);

        window.show_all();
    });

    application.run(&[]);
}
