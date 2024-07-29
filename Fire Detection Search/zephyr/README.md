This project's objective is to create peer to peer connection between 2 LoRa devices.

We use zephyr to build and flash the application. 

Prerequisites to run the application :

-Install zephyr (follow procedure from zephyr documentation)
-In your forder, copy the .west folder from zephyrproject
-Specify paths to zephyr files thanks to west config :
west config --local zephyr.base %HOMEPATH%\zephyrproject\zephyr
west config --local manifest.path %HOMEPATH%\zephyrproject\zephyr

At this point you should be able to build your code with west : 
west build -p always -b lora_e5_dev_board .

And then flash the code : 
west flash

If needed, you can setup a VSCode environnement which should help you debug the code easily : 

Add a .vscode folder to your app folder, and specify information for the tasks, settings, and launch .json files. Using Cortex-debug extension of VSCode.
You can follow this tutorial : https://medium.com/home-wireless/vscode-and-the-lora-e5-part-3-eb5238a40a72

If you setup this project on 2 LoRa devices to establish peer to peer communication, dont forget to match the LoRa parameters of sender and receiver.
