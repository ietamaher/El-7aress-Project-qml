# Qt6/QML Project Structure

This document outlines the structure of the Qt6/QML GStreamer application.

## Project Layout

The repository is organized into the following directories:

```
.
├── QT6-gstreamer-example.pro  # qmake project file
├── qml/                         # QML source files
│   ├── components/              # Reusable QML components
│   └── views/                   # Application views or screens
├── resources/                   # Resource files (e.g., .qrc)
└── src/                         # C++ source code
    ├── controllers/             # Application logic and flow control
    ├── models/                  # Data models and view models
    ├── services/                # Singleton services (e.g., ServiceManager)
    └── video/                   # GStreamer video integration
```

## C++ Backend (`src/`)

The C++ source code is located in the `src/` directory and is organized by function:

-   **`controllers/`**: Contains classes that manage application logic, user input, and view transitions. The `ApplicationController` is the main controller, orchestrating the interactions between the other controllers.
-   **`models/`**: Holds data models and view models that expose data and state to the QML frontend. This includes `MenuViewModel` for managing menu state and `OsdViewModel` for the on-screen display.
-   **`services/`**: Provides singleton services that are available throughout the application. The `ServiceManager` is a central point for accessing shared services.
-   **`video/`**: Includes the GStreamer integration code, such as the `GstVideoSource` and `VideoImageProvider`, which are responsible for capturing and displaying the video feed.

## QML Frontend (`qml/`)

The QML user interface is in the `qml/` directory, which is divided into:

-   **`components/`**: Reusable QML components, such as `OsdOverlay.qml`, that can be used across multiple views.
-   **`views/`**: Application screens or views, such as `main.qml` and `MainMenu.qml`.

## Build System (`.pro` file)

The project uses `qmake` as its build system. The `QT6-gstreamer-example.pro` file defines the project's configuration, including:

-   The C++ source and header files.
-   The QML files to be included in the application's resources.
-   The Qt modules to be linked against.
-   The GStreamer libraries and include paths.

## Resources (`.qrc` file)

The `resources/resources.qrc` file is a Qt resource file that bundles the QML files into the application's binary. This allows the QML files to be accessed at runtime using the `qrc:/` scheme.
