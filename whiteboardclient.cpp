#include <iostream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <SFML/Graphics.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <SFML/Network.hpp>
#include <sstream>
#include <string>
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
        this->sprite.setPosition({650.f, 10.f});
        this->sprite.setScale({0.1f, 0.1f});

        this->redotexture.loadFromFile("C:\\Users\\sairam\\Documents\\web\\college\\redo.png");
        this->redobtn.setTexture(this->redotexture);
        this->redobtn.setPosition({700.f, 10.f});
        this->redobtn.setScale({0.1f, 0.1f});
    }

public:
    bool is_Mouse_hover(RenderWindow &window)
    {
        Vector2i mouse_pos = Mouse::getPosition(window);
        FloatRect getbounds = sprite.getGlobalBounds();
        return getbounds.contains(static_cast<Vector2f>(mouse_pos));
    }
    void update_color(RenderWindow &window)
    {
        if (is_Mouse_hover(window))
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

    bool is_Mouse_hover_redo(RenderWindow &window)
    {
        Vector2i mouse_pos = Mouse::getPosition(window);
        FloatRect getbounds = this->redobtn.getGlobalBounds();
        return getbounds.contains(static_cast<Vector2f>(mouse_pos));
    }
    void update_color_redo(RenderWindow &window)
    {
        if (is_Mouse_hover_redo(window))
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
    vector<vector<Vertex>> total_lines;
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
            canvas[y][x].color = color; // not needed tbh as of now
            lines.push_back(Vertex(Vector2f(x, y), color));
        }
    }

    void finalize_line()
    {
        if (!lines.empty())
        {
            total_lines.push_back(lines);
            lines.clear(); // we push the previous in vector and then clear the current for new actions
        }
    }
    void draw(RenderWindow &window)
    {
        update_color(window);
        update_color_redo(window);
        window.draw(this->sprite);
        window.draw(this->redobtn);
        for (auto &line : total_lines) // draw previous lines in vector
        {
            if (!line.empty())
            {
                window.draw(&line[0], line.size(), LinesStrip);
            }
        }
        if (!lines.empty()) // draw line in current state
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


string serialize_vertices(const vector<Vertex> &vertices)
{
    ostringstream os;
    for (const auto &vertex : vertices)
    {
        os << vertex.position.x << "," << vertex.position.y << ","
            << static_cast<int>(vertex.color.r) << ","
            << static_cast<int>(vertex.color.g) << ","
            << static_cast<int>(vertex.color.b) << ","
            << static_cast<int>(vertex.color.a) << ";";
    }
    return os.str();
}

vector<Vertex> deserialize_vertices(const string &data)
{
    vector<Vertex> vertices;
    istringstream iss(data);
    string vertex_data;

    while (getline(iss, vertex_data, ';'))
    {
        istringstream vss(vertex_data);
        string part;
        vector<float> values;

        while (getline(vss, part, ','))
        {
            values.push_back(stof(part));
        }

        if (values.size() == 6) // x, y, r, g, b, a
        {
            Vertex vertex;
            vertex.position = Vector2f(values[0], values[1]);
            vertex.color = Color(static_cast<Uint8>(values[2]), static_cast<Uint8>(values[3]),
                                 static_cast<Uint8>(values[4]), static_cast<Uint8>(values[5]));
            vertices.push_back(vertex);
        }
    }

    return vertices;
}

void send_vertices(SOCKET socket, const std::vector<Vertex> &vertices)
{
    string serialized_data = serialize_vertices(vertices);
    send(socket, serialized_data.c_str(), serialized_data.size(), 0);
}

void receive_and_update(SOCKET socket, Canvas &canvas)
{
    char buffer[1024];
    while (true)
    {
        int bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0)
        {
            string data(buffer, bytes_received);
            vector<Vertex> received_vertices = deserialize_vertices(data);
            canvas.total_lines.push_back(received_vertices);
        }
        else
        {
            closesocket(socket);
            break;
        }
    }
}



void poll_events(RenderWindow &window,SOCKET client_socket, stack<Drawing_Action> &undo, stack<Drawing_Action> &redo, Action_graph &actionGraph, Canvas &canvas)
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
        else if (ev.type == Event::MouseMoved)
        {
            if (Mouse::isButtonPressed(Mouse::Left) && !canvas.is_Mouse_hover(window) && !canvas.is_Mouse_hover_redo(window))
            {
                isDrawing = true;
                Vector2i mouse_pos = Mouse::getPosition(window);
                Drawing_Action action;
                action.affected_area.push_back(mouse_pos);
                action.color = Color::Red;
                canvas.set_pixel(action.color, mouse_pos.x, mouse_pos.y);
                undo.push(action);
                while (!redo.empty())
                {
                    redo.pop();
                }
                actionGraph.Add_action(action);
            }
        }
        else if (ev.type == Event::MouseButtonReleased)
        {
            if (ev.mouseButton.button == Mouse::Left && isDrawing)
            {
                isDrawing = false;
                canvas.finalize_line();
                send_vertices(client_socket,canvas.total_lines.back());
            }
        }
        else if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left)
        {
            if (canvas.is_Mouse_hover(window)) // Check if the undo sprite is clicked
            {
                if (!undo.empty())
                {
                    Drawing_Action last_action = undo.top();
                    undo.pop();
                    redo.push(last_action);
                    actionGraph.undo_action(canvas, last_action);
                }
            }
            else if (canvas.is_Mouse_hover_redo(window)) // Check if the redo sprite is clicked
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



int main()
{
    RenderWindow window(VideoMode(800, 600), "Drawing Board");
    Canvas canvas(800, 600);
    stack<Drawing_Action> undo;
    stack<Drawing_Action> redo;
    Action_graph actions;
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        cout << "WSAStartup failed: " << result << endl;
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET)
    {
        cout << "Socket creation failed." << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    InetPtonW(AF_INET, L"127.0.0.1", &server_addr.sin_addr); 

    if (connect(client_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        cout << "Connection failed." << endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    thread(receive_and_update, client_socket, std::ref(canvas)).detach();
    while (window.isOpen())
    {
        poll_events(window,client_socket, undo, redo, actions, canvas);
        window.clear(Color::White);
        canvas.draw(window);
        window.display();
    }
    return 0;
}