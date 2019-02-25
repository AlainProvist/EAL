# EAL
## Eidolon Auto Link for Aura Kingdom

**This code is released for educational purpose only** under GNU GPL v3 licence.

## Description
EAL is a self contained minibot automatically **managing your eidolons**. It will automatically **talk** to them when possible to regain APs, **link** when available and **retrieve items** if the inventory is not full.

## Features
- Standalone auto **injector** targeting AK game process
  - Monitor all running instance of the game and auto inject listed DLLs
  - Ejects injected DLLs on closing
  - Monitor a list of running process that could prevent the injection from succeeding and promp for killing them
- Separate **EAL DLL** injected in the game process
  - Eidolon Auto Link features :
    - **Auto talk**
    - **Auto link**
    - **Auto retrieve objects**
  - DLL **detection protection**
  - **Gameguard** thread killer
  - **Multiclient** feature
  
## Repository content
This project can be compiled using **Visual Studio 2017** or **Visual Studio Code** (using VS2017 compiler). Both project files are provided here.
The code is using some hardoded **byte patterns** that will potentially need to be changed if the client code changes too much after some **future game update**. It currently works with **official FR/US/DE clients** of the game. Feel free to fork this repository and update the patterns or try to find patterns working on every clients, or even add some client detection to switch patterns accordingly if needed.


### Please do not ask for more features or maintaining the patterns.
I'm providing these sources as a **tribute to Skandia Bot project** to provide some **basic knowledge** about **internally hooking** a game and using its own code to add new features.
Note thats the methods I used are totally home made and that there are multiple different ways to do the same things (like triggering an exception at a specific code location to gain access to the registers and the stack and modify them).
