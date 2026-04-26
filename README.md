# Inventory & Equipment System (Unreal Engine 5)

This project is a modular inventory and equipment system developed in Unreal Engine 5 using C++.

## About

This project was created as part of a course on Udemy by Stephen Ulibarri, with the goal of improving my skills in Unreal Engine and C++.

The course provided the foundation, but during development I focused on deeply understanding the systems and extending them beyond the basic implementation.

---

## What I Learned

- Unreal Engine architecture (Actors, Components, Subsystems)
- C++ gameplay programming
- Modular system design using fragments
- Working with UMG (UI)
- Gameplay Tags usage
- Event-driven architecture (delegates)
- Replication using Fast Array
- Debugging and system validation

---

## Features

### Inventory System
- Grid-based inventory
- Item pickup using line trace
- Stackable items
- Splitting stacks
- Drag & drop support
- Item interaction pop-up
- Dropping items into the world

---

### Item System

Each item is built using a **data-driven approach**:

#### Item Manifest
- Stores item data
- Defines item type and category
- Contains fragments

#### Item Fragments
Used to modularly define behavior:
- Grid size
- Icon
- Stackable logic
- Consumable logic
- Equipment logic

---

### Equipment System

- Equip / Unequip items
- Equipment slots
- Spawning equipment actors
- Attaching to character skeletal mesh (sockets)
- Fully fragment-driven configuration

---

### UI (UMG)

- Inventory grid
- Drag & drop system
- Item interaction menu
- HUD integration

---

### Technical Features

- Gameplay Tags integration
- Fast Array replication (network-ready)
- Clean modular architecture
- Separation of data and logic

---

## Project Structure

- `InventoryComponent` – core logic  
- `ItemManifest` – item data  
- `ItemFragments` – modular behavior system  
- `EquipmentComponent` – equip logic  
- `UI Widgets` – user interface  
- `EquipActor` – spawned equipment actors  

---

## Author

Yehor 
