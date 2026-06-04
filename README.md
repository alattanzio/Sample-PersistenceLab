# Sample-PersistenceLab
This is a sample project to accompany the Unreal Fest Chicago 2026 talk on saving world state. Basic maps demonstrate common gameplay saving needs for actors, instanced actors and Mass entities. Those maps can be traveled in-between and can be saved and restored at any point. Map placed actors get their properties restored. Spawned actors and Mass entities get respawned with their state from the previous session. A SaveGame class, world subsystem and game instance subsystem together coordinate storing and reapplying state for the persistent levels and (world partition) streaming levels.

# Featured UE plugins
Level Streaming Persistence Plugin (experimental) - available since UE 5.3 with new features incoming in UE 5.8.
Instanced Actors Plugin (experimental) - available since UE 5.4 and used in shipped Epic titles.
Mass Entity framework - production ready as of UE 5.2, though Mass gameplay and AI plugins remain experimental.

# Useful resources
Incoming: The Player was Here - presentation and text version
Presentation at Unreal Day 2026 in Belgrade, Serbia and Unreal Fest Chicago 2026. Text version of the talk will be available right after Unreal Fest (June 2026). Recording of Unreal Fest talk will likely be uploaded later this year.

Incoming: Your First 60 Minutes with Instanced Actors
I'm preparing a text tutorial for Instanced Actors. Check back late June 2026.

Your First 60 Minutes with Mass by James Keeling
https://dev.epicgames.com/community/learning/tutorials/6vG6/unreal-engine-your-first-60-minutes-with-mass
