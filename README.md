# Embedded Systems Development Project - Hiking Tour Assistant
## Project description
The projext implements a smartwatch UI for the LilyGo TWatch 2020, which allows the user to start and stop recording sessions and a web server for a Raspberry Pi for displaying the session statistics. During a hiking session travelled distance and step count are displayed and saved to the smartwatch. In addition, the web UI for a Raspberry Pi can display extended session statistics, including burned calories. The session data can be synchronized between the smartwatch and the Raspberry Pi using a Bluetooth connection.

## How to Install and Run the Project
The development was done on Windows using VSCode and PlatformIO. Using the same environment is highly recommended. 

- [VSCode](https://code.visualstudio.com/)
- [PlatformIO installation guide](https://platformio.org/install/ide?install=vscode)

After installing PlatformIO, you should be able to see the following tab:

![image](https://user-images.githubusercontent.com/104350901/226669954-61ad5254-234a-4c57-afc4-fab5ff5dd839.png)

Click ‘Pick a folder’ and choose …\TTGo_FW\Watch_TTGo_fw folder, which can be cloned from the GitHub repository. The folder contains a ‘platformio.ini’ which initializes the project automatically. (NOTE: we had some issues running the Project from a folder connected to OneDrive, so we recommend saving the project folder to a physical drive) 

After the initialization of the project, you should see the following screen: 

![image](https://user-images.githubusercontent.com/128503048/226716839-e24ec822-3a42-4cd5-9f63-f7c7d2356682.png)

Here you can click on _Build_, to build an image of the project. This image can be then pushed to the LilyGo TWatch by clicking _Upload_ assuming that the watch is connected with a USB-cable.  

The library dependencies can be found in .pio/libdeps/ttgo-t-watch and they include: 

- TTGO TWatch Library 
-	LittleFS_esp32 

### RPi program:

The RPi program can be found in the 'RPi' folder. The folder is organized as follows:

- The root folder contains the Python code files and requirements information
- User data is saved in the 'data' folder
- HTML files used by the web server are in the 'templates' folder

![image](https://user-images.githubusercontent.com/128503048/226718933-8d5ffcac-fabc-4195-9155-97a02ed982b2.png)

The program is divided into two parts: synchronization and web server. The data used by the system is in the 'data' folder. It includes a file called ‘session’, which is where the data from the latest session is saved, and a file called 'user', where the user’s weight is saved.

The synchronization program is implemented in the _bt.py_, _hike.py_ and _receiver.py_ files. The _bt.py_ file includes functionality for establishing the Bluetooth connection with the smartwatch and receiving session data from it. The _hike.py_ file includes a class representing a hiking session and some functions related to it. The _receiver.py_ file includes functionality for processing the session data and a main function to run the synchronization program. To execute the synchronization program, run _receiver.py_ on the command line with the command 'python3 receiver.py'.

The web server is implemented in the _app.py_ file. In addition, several HTML files corresponding to the different web pages can be found in the 'templates' folder. The web server can be run by running the _app.py_ file.

The development was done on the RPi itself using Thonny as an IDE. It is also possible to develop on another platform using any IDE with Python support, such as. The program has the following dependencies:

-	Python 3.9.2 or newer
-	Flask 2.2.2
-	PyBluez 0.23
-	Pillow 8.1.2

## How to Use the Project

## Collaborators
Ville Synkkänen

Pyry Aho

Seppo Koivisto
