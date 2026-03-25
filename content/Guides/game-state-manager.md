## Game State Manager {#gsm}

### Description
The Game State Manager (GSM)'s concept is similar as the stuff taught in Game Implementation Techniques (GIT). Can see [Lecture 3](https://distance3.sg.digipen.edu/2026sg-spring/pluginfile.php/52555/mod_resource/content/1/Lecture3_GameStateManager_FrameRateController_ppt%20[PDF%20format].pdf#page=10) 

Some differences:
- uses C++ classes instead of c-style function pointers
- it uses [RAII](https://en.cppreference.com/w/cpp/language/raii.html) design pattern (to summary - Resources managed in Constructor and Deconstructor)

#### Renamed functions:

| From GIT   | Current GSM     |
| ---------- | --------------- |
| Load       | *Constructor*   |
| Initialize | Init            |
| Update     | Update          |
| Draw       | Render          |
| Free       | Exit            |
| Unload     | *Deconstructor* |

### Flowchart
![](GSM_Explanation.svg)

### Individual Functions
Similar to [GIT's Lecture 3 slides](https://distance3.sg.digipen.edu/2026sg-spring/pluginfile.php/52555/mod_resource/content/1/Lecture3_GameStateManager_FrameRateController_ppt%20[PDF%20format].pdf#page=10) 

#### Constructor

| Info              | Details                                                                               |
| ----------------- | ------------------------------------------------------------------------------------- |
| Stuff to put here | Load necessary data<br>Example:<br>- Load textures/fonts<br>- Create (`new`) objects  |
| When it's called  | *C++ Constructor*<br>Called at the start of state<br>- **NOT called** when restarting |
#### Init

| Info                  | Details                                                                      |
| --------------------- | ---------------------------------------------------------------------------- |
| Stuff to put here     | Prepares data to be used<br>Example: <br>	- Spawn enemies<br>	- Reset player |
| When it's called <br> | After Constructor<br>- **Called** when restarting                            |
#### Update

| Info                  | Details                                                                              |
| --------------------- | ------------------------------------------------------------------------------------ |
| Stuff to put here     | Update systems/objects<br>Example:<br>- Update user input<br>- Update player/enemies |
| When it's called <br> | Every frame until GSM::ChangeScene() is called<br>                                   |
#### Render

| Info                  | Details                                            |
| --------------------- | -------------------------------------------------- |
| Stuff to put here     | Render objects                                     |
| When it's called <br> | Every frame until GSM::ChangeScene() is called<br> |
#### Exit

| Info                  | Details                                              |
| --------------------- | ---------------------------------------------------- |
| Stuff to put here     | Clean up objects<br>Example:<br>- Despawn enemies    |
| When it's called <br> | Before Deconstructor<br>- **Called** when restarting |
#### Deconstructor

| Info                  | Details                                                                                      |
| --------------------- | -------------------------------------------------------------------------------------------- |
| Stuff to put here     | Free Objects/Resources<br>For example:<br>- Free Textures/Fonts<br>- Free (`delete`) objects |
| When it's called <br> | *C++ Deconstructor*<br>Called when state is terminating, end of state                        |




