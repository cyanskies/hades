# Hades

NOTE: this library is still in early development.
Hades is a 2d game framework library containing many utilities, classes and subsystems for use in games.

# Synopsis

Requires C++17

Examples are held in the hades-examples repository

# Features
Parallel Jobs:
A parallel task subsystem for managing many small tasks across multiple threads.

Console:
A console command parser and property storage object. This acts as the backend for a quake style game console. 
It supports many of the expected console functions, running commands, setting named properties that the game can use,
and rebinding input commands.

Archive Stream:
A SFML compatable input stream that reads from zipped archives.

Data Manager:
A resource loader that loads files, properties and game values from game manifest files.

Curves:
Curve based value storage, allowing access to the complete history of the value, and for interpolated values.

App:
An application class that uses the framework utilities to manage game states, frame timing and other common game tasks.
NOTE: this class is automatically instantiated with a provided main() unless HADES_NO_MAIN is defined during compilation.

Transactional storage:
Provides multithreaded wrappers for arbitary objects, providing value guarded or shared mutex semantics.
Also provides a transactional map class that locks the map a short as possible and only locks individual entries to update them.

property_bag: 
a type erased map, capable of storing any copiable value and retrieving it.

# modules

## hades-util
Contains utilities used by the library, but not related to the goal of the library

## hades-basic
Contains classes, interfaces and functions that can be implemented only with standard C++

## hades-core
Contains the remaining library features that depend on zlib, yaml-cpp, sfml and sf-gui.

## hades-app
A full engine application built using the library

# hades example

See the /test project for a simple example of building hades and starting a user specified gamestate.
