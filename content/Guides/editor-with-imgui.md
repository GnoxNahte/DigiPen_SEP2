## Editor with imgui {#editor-with-imgui}

ImGui Links:
- [GitHub](https://github.com/ocornut/imgui)
- [ImGui Docs](https://github.com/ocornut/imgui/wiki)

### Setup
1. Inherit from Inspectable (Location at `Editor/EditorUtils.h`)
2. Override `DrawInspector()`
3. Override `CheckIfClicked(const AEVec2& mousePos)` (Optional for now, might make it required later?)

### Example
In .h
```cpp
class Player : public Inspectable {
	// other attributes / functions ...
	
public:
    void DrawInspector() override;
    bool CheckIfClicked(const AEVec2& mousePos) override;
}
```

In .cpp
```cpp
void Player::DrawInspector()
{
    ImGui::Begin("Player"); // Creates a window called "Player"
	
	ImGui::SliderInt("Health", &health, 0, stats.maxHealth); 
    ImGui::DragFloat2("Position", &position.x, 0.1f); 
    
    ImGui::End(); // Ends the window player
}

bool Player::CheckIfClicked(const AEVec2& mousePos)
{
	// Can implement your own collision checking.
	// Like can be circle collision too.
    return  fabsf(position.x - mousePos.x) < stats.playerSize.x &&
            fabsf(position.y - mousePos.y) < stats.playerSize.y;
}
```
