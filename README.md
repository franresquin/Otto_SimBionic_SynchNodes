# Otto_SimBionic_SynchNodes #
Code for the ESP-based nodes used to synchronize the Tobi Glass with the Gait motion capture system (Ottobock gait analysis Lab).  

The "trigger_node" receives the trigger from the gait motion capture system and sends it wireless to the "glass_node".  

The "glass_node" receives the message from the "trigger_node" and transforms this information to a TTL compatible signal that it will capture by the Glass recording box of the glass system.
