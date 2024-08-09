#include <iostream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

using namespace sf;
using namespace std;

struct Pixel
{
    Color color;
};
struct Drawing_Action
{
    vector<Vector2i> affected_area;
    Color color;
};

class Canvas
{
    vector<vector<Pixel>> canvas;
    int width;
    int height;

    Sprite sprite;
    Texture texture;

    Sprite redobtn;
    Texture redotexture;

    void init_gui()
    {
        this->texture.loadFromFile("C:\\Users\\sairam\\Downloads\\undo.png");
        this->sprite.setTexture(this->texture);
        this->sprite.setPosition({650.f,10.f});
        this->sprite.setScale({0.1f,0.1f});

        this->redotexture.loadFromFile("C:\\Users\\sairam\\Documents\\web\\college\\redo.png");
        this->redobtn.setTexture(this->redotexture);
        this->redobtn.setPosition({700.f,10.f});
        this->redobtn.setScale({0.1f,0.1f});
    }


public:
    bool is_Mouse_hover(RenderWindow& window)
   {
    Vector2i mouse_pos=Mouse::getPosition(window);
    FloatRect getbounds=sprite.getGlobalBounds();
    return getbounds.contains(static_cast<Vector2f>(mouse_pos));
   }
  void update_color(RenderWindow& window)
  {
    if(is_Mouse_hover(window))
    {
        // sprite.setColor(Color::Cyan);
        this->texture.loadFromFile("C:\\Users\\sairam\\Documents\\web\\college\\undohover.png");
        sprite.setTexture(this->texture);
    }
    else
    {
        this->texture.loadFromFile("C:\\Users\\sairam\\Downloads\\undo.png");
        this->sprite.setTexture(this->texture);
        // sprite.setColor(Color::Black);
    }
  }

    bool is_Mouse_hover_redo(RenderWindow& window)
   {
    Vector2i mouse_pos=Mouse::getPosition(window);
    FloatRect getbounds=this->redobtn.getGlobalBounds();
    return getbounds.contains(static_cast<Vector2f>(mouse_pos));
   }
  void update_color_redo(RenderWindow& window)
  {
    if(is_Mouse_hover_redo(window))
    {
        // sprite.setColor(Color::Cyan);
        this->redotexture.loadFromFile("C:\\Users\\sairam\\Documents\\web\\college\\redohover.png");
        this->redobtn.setTexture(this->redotexture);
    }
    else
    {
        this->redotexture.loadFromFile("C:\\Users\\sairam\\Documents\\web\\college\\redo.png");
        this->redobtn.setTexture(this->redotexture);
        // sprite.setColor(Color::Black);
    }
  }
    vector<Vertex> lines;
    vector<vector<Vertex>>total_lines;
    Canvas(int width, int height)
    {
        this->width = width;
        this->height = height;
        this->canvas.resize(this->height, vector<Pixel>(this->width, {Color::White}));
        init_gui();
    }

    void set_pixel(Color color, int x, int y)
    {
        if (x >= 0 && x < this->width && y >= 0 && y < this->height)
        {
            canvas[y][x].color = color; //not needed tbh as of now   
            lines.push_back(Vertex(Vector2f(x, y), color));
        }
    }

    void finalize_line()
    {
        if(!lines.empty())
        {
            total_lines.push_back(lines);
            lines.clear();  //we push the previous in vector and then clear the current for new actions 
        }
    }
    void draw(RenderWindow &window)
    {
       update_color(window);
       update_color_redo(window);
        window.draw(this->sprite);
        window.draw(this->redobtn);
        for(auto& line:total_lines)  //draw previous lines in vector
        {
            if(!line.empty())
            {
                window.draw(&line[0],line.size(),LinesStrip);
            }
        }
        if(!lines.empty()) //draw line in current state
        {
            window.draw(&lines[0], lines.size(), LinesStrip);
        }
    }
};

class Action_graph
{
    vector<Drawing_Action> actions;

public:
    void Add_action(Drawing_Action &action)
    {
        actions.push_back(action);
    }

    void undo_action(Canvas &canvas, Drawing_Action last_action)
    {
        for (auto &pixel_pos : last_action.affected_area)
        {
            canvas.set_pixel(Color::White, pixel_pos.x, pixel_pos.y);
        }
        // actions.push_back(last_action);
    }

    void redo_action(Canvas &canvas, Drawing_Action action)
    {
        for (auto &pixel_pos : action.affected_area)
        {
            canvas.set_pixel(action.color, pixel_pos.x, pixel_pos.y);
        }
        actions.push_back(action);
    }
};
void poll_events(RenderWindow &window, stack<Drawing_Action> &undo, stack<Drawing_Action> &redo, Action_graph &actionGraph, Canvas &canvas)
{
    Event ev;
    Vector2i last_mouse_pos;
    static bool isDrawing = false;
    while (window.pollEvent(ev))
    {
        if (ev.type == Event::Closed)
        {
            window.close();
        }
        else if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape)
        {
            window.close();
        }
        else if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Z)
        {
            if (!undo.empty())
            {
                Drawing_Action last_action = undo.top();
                undo.pop();
                redo.push(last_action);
                actionGraph.undo_action(canvas, last_action);
            }
        }
        else if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Y)
        {
            if (!redo.empty())
            {
                Drawing_Action last_action = redo.top();
                redo.pop();
                undo.push(last_action);
                actionGraph.redo_action(canvas, last_action);
            }
        }
        else if(ev.type==Event::MouseMoved)
        {
            if(Mouse::isButtonPressed(Mouse::Left) && !canvas.is_Mouse_hover(window) && !canvas.is_Mouse_hover_redo(window))
            {
                isDrawing=true;
                Vector2i mouse_pos=Mouse::getPosition(window);
                Drawing_Action action;
                action.affected_area.push_back(mouse_pos);
                action.color=Color::Red;
                canvas.set_pixel(action.color,mouse_pos.x,mouse_pos.y);
                undo.push(action);
                while(!redo.empty())
                {
                    redo.pop();
                }
                actionGraph.Add_action(action);
            }
        }
        else if(ev.type==Event::MouseButtonReleased)
        {
            if(ev.mouseButton.button==Mouse::Left && isDrawing)
            {
                isDrawing=false;
                canvas.finalize_line();
            }
        }
        else if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left)
        {
            if (canvas.is_Mouse_hover(window))  // Check if the undo sprite is clicked
            {
                if (!undo.empty())
                {
                    Drawing_Action last_action = undo.top();
                    undo.pop();
                    redo.push(last_action);
                    actionGraph.undo_action(canvas, last_action);
                }
            }
            else if (canvas.is_Mouse_hover_redo(window))  // Check if the redo sprite is clicked
            {
                if (!redo.empty())
                {
                    Drawing_Action last_action = redo.top();
                    redo.pop();
                    undo.push(last_action);
                    actionGraph.redo_action(canvas, last_action);
                }
            }
        }

    }
}

void main_engine()
{
    RenderWindow window(VideoMode(800, 600), "Drawing Board");
    Canvas canvas(800, 600);
    stack<Drawing_Action> undo;
    stack<Drawing_Action> redo;
    Action_graph actions;
    while (window.isOpen())
    {
        // canvas.update_color(window);
        poll_events(window, undo, redo, actions, canvas);
        window.clear(Color::White);
        canvas.draw(window);
        window.display();
    }
}

int main()
{
    RenderWindow window(VideoMode(800, 600), "Drawing board");
    main_engine();

    return 0;
}