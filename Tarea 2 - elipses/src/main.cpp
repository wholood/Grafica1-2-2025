#include "PixelRender.h"
#include <iostream>
#include <cmath>
#include <vector>

class CMyTest : public CPixelRender
{
public:
    enum class EDrawingMode
    {
        Line,
        Ellipse
    };

    struct LineData
    {
        int x0, y0;
        int x1, y1;
        RGBA color;
    };

    std::vector<LineData> m_lines;
    
    int m_x0 = -1;
    int m_y0 = -1;
    int m_x1 = -1;
    int m_y1 = -1;

    //Atributo para el Checkbox
    bool m_useRealArithmetic = false;

    struct EllipseData
    {
        int cx, cy;
        long long a, b;
        RGBA color;
    };

    std::vector<EllipseData> m_ellipses; // Vector para guardar las elipses
    EDrawingMode m_drawingMode = EDrawingMode::Line;

    // Valores iniciales para la elipse de prueba
    int m_cx = 256;
    int m_cy = 256;
    int m_a = 150; // Radio en X
    int m_b = 100; // Radio en Y
    
    RGBA m_current_color = { 255, 255, 255, 255 };
    
    int m_framesThisSecond = 0;

    CMyTest() {};
    ~CMyTest() {};

    void drawLine(int x0, int y0, int x1, int y1, RGBA color)
    {
        // No dibujamos nada si los puntos no son válidos
        if (x0 == -1 || y0 == -1 || x1 == -1 || y1 == -1)
            return;

        // Calculamos las diferencias absolutas en x e y
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);

        // Determinamos la dirección
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        int err = dx - dy;

        while (true)
        {
            // Dibujamos el pixel actual, verificamos límites para no salirnos del buffer
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
            {
                setPixel(x0, y0, color);
            }

            // Condición de parada
            if (x0 == x1 && y0 == y1)
                break;

            // d * 2
            int e2 = err<<1;

            // Verificamos el error para decidir el siguiente paso

            if (e2 > -dy) // d>0 mover en x 
            {
                err -= dy; 
                x0 += sx;  
            }

            if (e2 < dx) // d<0 mover en y
            {
                err += dx; 
                y0 += sy;  
            }
            // Si la pendiente es exactamente 1 o -1, ambas condiciones se cumplen y el algoritmo avanza en diagonal (un paso en x y uno en y).
        }
    }

    void drawLineReal(int x0, int y0, int x1, int y1, RGBA color)
    {
        if (x0 == -1 || y0 == -1 || x1 == -1 || y1 == -1)
            return;

        int dx = x1 - x0;
        int dy = y1 - y0;

        // calculams el número de pasos basándonos en el eje dominante
        int steps = std::max(std::abs(dx), std::abs(dy));

        // un solo punto
        if (steps == 0)
        {
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
                setPixel(x0, y0, color);
            return;
        }

        // Calculamos  los incrementos m y 1/m en punto flotante
        double x_inc = (double)dx / (double)steps;
        double y_inc = (double)dy / (double)steps;

        // punto inicial
        double x = (double)x0;
        double y = (double)y0;

        for (int i = 0; i <= steps; ++i)
        {
            int draw_x = (int)std::round(x);
            int draw_y = (int)std::round(y);

            if (draw_x >= 0 && draw_x < width && draw_y >= 0 && draw_y < height)
            {
                setPixel(draw_x, draw_y, color);
            }

            x += x_inc; // x = x + (1/m)
            y += y_inc; // y = y + m
        }
    }

    void drawEllipsePoints(int x, int y, long long cx, long long cy, RGBA color)
    {
        // Verificamos límites para los 4 puntos
        if (cx + x >= 0 && cx + x < width && cy + y >= 0 && cy + y < height)
            setPixel(cx + x, cy + y, color);
        if (cx - x >= 0 && cx - x < width && cy + y >= 0 && cy + y < height)
            setPixel(cx - x, cy + y, color);
        if (cx - x >= 0 && cx - x < width && cy - y >= 0 && cy - y < height)
            setPixel(cx - x, cy - y, color);
        if (cx + x >= 0 && cx + x < width && cy - y >= 0 && cy - y < height)
            setPixel(cx + x, cy - y, color);
    }

    void drawEllipse1(int cx, int cy, long long a, long long b, RGBA color)
    {
        // --- Región 1 (Modalidad 1) 
        long long x = 0;
        long long y = b;
        long long d = b * ((4 * b) - (4 * a * a)) + a * a;

        while (b * b * 2 * (x + 1) < a * a * (2*y-1))
        {
            drawEllipsePoints(x, y, cx, cy, color);

            if (d > 0)
            {
                d = d + 4 * (b * b * (2 * x + 3) + a * a * (-2 * y + 2));
                y--;
            }
            else {
                d = d + 4 * (b * b * (2 * x + 3));
            }
            x++; 
        }

        // --- Región 2 (Modalidad 2)
        d= b * b * (4 * x * x + 4 * x + 1) + a * a * (4 * y * y - 8 * y + 4) - 4 * a * a * b * b;
        while (y >= 0)
        {
            drawEllipsePoints(x, y, cx, cy, color);
            if (d <= 0)
            {
                d= d + 4 * (b * b *(2 * x + 2) + a * a * (-2 * y + 3));
                x++;
            }
            else
            {
               d = d + 4 * a * a * (-2 * y + 3);
            }
            y--;
        }
    }

    void update()
    {
        // Limpiamos el buffer a color negro transparente
        std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        //Dibujamos todas las líneas guardadas en el vector
        for (const LineData& line : m_lines)
        {
            if (m_useRealArithmetic)
                drawLineReal(line.x0, line.y0, line.x1, line.y1, line.color);
            else
                drawLine(line.x0, line.y0, line.x1, line.y1, line.color);
        }

        // Dibujamos la línea actual
        if (m_useRealArithmetic)
            drawLineReal(m_x0, m_y0, m_x1, m_y1, m_current_color);
        else
            drawLine(m_x0, m_y0, m_x1, m_y1, m_current_color);

        // Dibujamos todas las elipses guardadas en el vector
        for (const EllipseData& ellipse : m_ellipses)
        {
            drawEllipse1(ellipse.cx, ellipse.cy, ellipse.a, ellipse.b, ellipse.color);
        }

        // Dibujamos la elipse interactiva SÓLO si estamos en modo Elipse
        // Y ADEMÁS el botón izquierdo del mouse está presionado.
        if (mouseButtonsDown[GLFW_MOUSE_BUTTON_LEFT] && m_drawingMode == EDrawingMode::Ellipse)
        {
            drawEllipse1(m_cx, m_cy, (long long)m_a, (long long)m_b, m_current_color);
        }
       
    }

    void drawInterface()
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;

        m_framesThisSecond++; // Contamos el frame actual

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        


        //Nuestra ventana de ImGui
        ImGui::SetNextWindowSize(ImVec2(350, 380), ImGuiCond_Once); // Aumentamos altura
        ImGui::Begin("Controles Tarea 1 y 2");

        // --- AÑADIR ESTO: SELECCIÓN DE MODO ---
        ImGui::Text("Modo de Dibujo");
        // Convertimos el enum a int* para ImGui
        // (ImGui::RadioButton trabaja con ints)
        int* p_mode = reinterpret_cast<int*>(&m_drawingMode);

        // Creamos los botones de radio
        // Usamos (int)EDrawingMode::Line y (int)EDrawingMode::Ellipse
        ImGui::RadioButton("Linea", p_mode, (int)EDrawingMode::Line);
        ImGui::SameLine();
        ImGui::RadioButton("Elipse", p_mode, (int)EDrawingMode::Ellipse);
        ImGui::Separator();
        // --- FIN DE AÑADIDOS ---

        ImGui::Text("Controles Tarea 1 - Líneas");
        ImGui::Checkbox("Usar Aritmetica Real", &m_useRealArithmetic);

        ImGui::Spacing();
        // ImGui::ColorEdit4 usa floats [0.0, 1.0] y m_current_color usa unsigned char [0, 255], creamos un array temporal para convertir
        float color_floats[4] = {
            m_current_color.r / 255.0f,
            m_current_color.g / 255.0f,
            m_current_color.b / 255.0f,
            m_current_color.a / 255.0f
        };

        if (ImGui::ColorEdit4("Color de Linea", color_floats))
        {
            m_current_color.r = (unsigned char)(color_floats[0] * 255.0f);
            m_current_color.g = (unsigned char)(color_floats[1] * 255.0f);
            m_current_color.b = (unsigned char)(color_floats[2] * 255.0f);
            m_current_color.a = (unsigned char)(color_floats[3] * 255.0f);
        }

        ImGui::Spacing();

        if (ImGui::Button("Agregar 1000 Lineas"))
        {
            for (int i = 0; i < 1000; ++i)
            {
                // Coordenadas aleatorias dentro de la ventana
                int x0 = rand() % width;
                int y0 = rand() % height;
                int x1 = rand() % width;
                int y1 = rand() % height;

                // Color aleatorio
                RGBA rand_color = {
                    (unsigned char)(rand() % 256), // R
                    (unsigned char)(rand() % 256), // G
                    (unsigned char)(rand() % 256), // B
                    255                           // A (sólido)
                };

                // Añadimos la línea al vector
                m_lines.push_back({ x0, y0, x1, y1, rand_color });
            }
            std::cout << "Añadidas 1000 lineas aleatorias. Total: " << m_lines.size() << std::endl;
        }

        ImGui::SameLine(); // Pone el siguiente widget en la misma línea

        if (ImGui::Button("Limpiar Todo"))
        {
            m_lines.clear();
            std::cout << "Lineas limpiadas." << std::endl;
        }

        
        ImGui::End();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (deltaTime >= 1.0) { // Si ha pasado un segundo
            char title[256];
            snprintf(title, sizeof(title), "CPixelRender - FPS: %d", m_framesThisSecond);
            glfwSetWindowTitle(m_window, title);

            // Reseteamos contadores
            pixelsModifiedThisSecond = 0;
            m_framesThisSecond = 0;
            lastTime = currentTime;
        }
    }

    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS)
        {
            std::cout << "Tecla " << key << " presionada\n";
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
        else if (action == GLFW_RELEASE)
            std::cout << "Tecla " << key << " soltada\n";
    }

    void onMouseButton(int button, int action, int mods)
    {
        ImGuiIO& io = ImGui::GetIO();
        // Si estamos sobre un widget, no hacemos nada. Evita creación de líneas al interactuar con el menú
        if (io.WantCaptureMouse)
            return;

        if (button >=0 && button < 3)
        {
            double xpos, ypos;
            glfwGetCursorPos(m_window, &xpos, &ypos);

            // Convertimos la coordenada Y del mouse (0,0 arriba-izquierda) a la coordenada del buffer (0,0 abajo-izquierda)
            int render_y = height - 1 - (int)ypos;

            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[button] = true;
                std::cout << "Mouse presionado en (" << xpos << ", " << ypos << ") -> Renderizado en (" << (int)xpos << ", " << render_y << ")\n";

                // --- LÓGICA DE MODO ---
                if (m_drawingMode == EDrawingMode::Line)
                {
                    // Modo Línea: Inicia la línea
                    m_x0 = (int)xpos;
                    m_y0 = render_y;
                    m_x1 = (int)xpos;
                    m_y1 = render_y;
                }
                else if (m_drawingMode == EDrawingMode::Ellipse)
                {
                    // Modo Elipse: Fija el centro
                    m_cx = (int)xpos;
                    m_cy = render_y;
                    m_a = 0; // Resetea radios para el "live preview"
                    m_b = 0;
                }
                // --- FIN LÓGICA DE MODO ---
            }
            else if (action == GLFW_RELEASE)
            {
                mouseButtonsDown[button] = false;
                std::cout << "Mouse soltado en (" << xpos << ", " << ypos << ") -> Renderizado en (" << (int)xpos << ", " << render_y << ")\n";

                // --- LÓGICA DE MODO ---
                if (m_drawingMode == EDrawingMode::Line)
                {
                    // Modo Línea: Finaliza y guarda la línea
                    m_x1 = (int)xpos;
                    m_y1 = render_y;

                    LineData finished_line = { m_x0, m_y0, m_x1, m_y1, m_current_color };
                    m_lines.push_back(finished_line);
                    std::cout << "Linea guardada. Total de lineas: " << m_lines.size() << std::endl;

                    // Reseteamos la línea interactiva
                    m_x0 = -1; m_y0 = -1; m_x1 = -1; m_y1 = -1;
                }
                else if (m_drawingMode == EDrawingMode::Ellipse)
                {
                    // Modo Elipse: Finaliza y guarda la elipse
                    // Los radios son la distancia absoluta desde el centro
                    m_a = (int)std::abs(xpos - m_cx);
                    m_b = (int)std::abs(render_y - m_cy);

                    // Solo guardamos si tiene un tamaño mínimo
                    if (m_a > 0 || m_b > 0)
                    {
                        EllipseData finished_ellipse = { m_cx, m_cy, (long long)m_a, (long long)m_b, m_current_color };
                        m_ellipses.push_back(finished_ellipse);
                        std::cout << "Elipse guardada. Total de elipses: " << m_ellipses.size() << std::endl;
                    }
                }
                // --- FIN LÓGICA DE MODO ---
            }
        }
    }

    void onCursorPos(double xpos, double ypos)
    {
        ImGuiIO& io = ImGui::GetIO();
        // Si estamos sobre un widget, no hacemos nada. Evita creación de líneas al interactuar con el menú
        if (io.WantCaptureMouse)
            return;

        if (mouseButtonsDown[GLFW_MOUSE_BUTTON_LEFT])
        {
            int render_y = height - 1 - (int)ypos; // Y invertida

            // --- LÓGICA DE MODO ---
            if (m_drawingMode == EDrawingMode::Line)
            {
                // Modo Línea: actualiza el punto final
                m_x1 = (int)xpos;
                m_y1 = render_y;
            }
            else if (m_drawingMode == EDrawingMode::Ellipse)
            {
                // Modo Elipse: actualiza los radios "en vivo"
                m_a = (int)std::abs(xpos - m_cx);
                m_b = (int)std::abs(render_y - m_cy);
            }
        }
    }
};

int main() {
    CMyTest test;
    if (!test.setup()) {
        fprintf(stderr, "Failed to setup CPixelRender\n");
        return -1;
    }

    test.mainLoop();

    return 0;
}
