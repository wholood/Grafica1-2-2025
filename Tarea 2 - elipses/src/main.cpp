#include "PixelRender.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

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

    // Variable para controlar qué algoritmo de elipse usar
    bool m_useOptimizedEllipse = false;
    
    RGBA m_current_color = { 255, 255, 255, 255 };
    
    int m_framesThisSecond = 0;

    bool m_isTesting = false; // Bandera para saber si estamos en modo test
    std::vector<std::pair<int, int>> m_capturedPixels; // Vector para guardar (x,y)

    CMyTest() {};
    ~CMyTest() {};

    void setPixel(int x, int y, const RGBA& color)
    {
        // Si estamos probando, guardamos la coordenada
        if (m_isTesting)
        {
            m_capturedPixels.push_back({ x, y });
        }

        // SIEMPRE llamamos al padre para que se vea en pantalla (requisito visual)
        CPixelRender::setPixel(x, y, color);
    }

    void drawLine(int x0, int y0, int x1, int y1, RGBA color)
    {
        // No dibujamos nada si los puntos no son v�lidos
        if (x0 == -1 || y0 == -1 || x1 == -1 || y1 == -1)
            return;

        // Calculamos las diferencias absolutas en x e y
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);

        // Determinamos la direcci�n
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        int err = dx - dy;

        while (true)
        {
            // Dibujamos el pixel actual, verificamos l�mites para no salirnos del buffer
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
            {
                setPixel(x0, y0, color);
            }

            // Condici�n de parada
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

        // calculams el n�mero de pasos bas�ndonos en el eje dominante
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
        // Verificamos l�mites para los 4 puntos
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
        // Radio B es 0 Dibuja una l�nea horizontal
        if (b == 0 && a > 0)
        {
            drawLine(cx - (int)a, cy, cx + (int)a, cy, color);
            return;
        }

        // Radio A es 0 Dibuja una l�nea vertical
        if (a == 0 && b > 0){
            drawLine(cx, cy - (int)b, cx, cy + (int)b, color);
            return;
        }


        // --- Regi�n 1 (Modalidad 1) 
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

        // --- Regi�n 2 (Modalidad 2)
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

    void drawEllipse2(int cx, int cy, long long a, long long b, RGBA color)
    {
        // --- Guard Clauses ---
        if (a == 0 && b == 0) {
            if (cx >= 0 && cx < width && cy >= 0 && cy < height) setPixel(cx, cy, color);
            return;
        }
        if (b == 0 && a > 0) {
            drawLine(cx - (int)a, cy, cx + (int)a, cy, color);
            return;
        }
        if (a == 0 && b > 0) {
            drawLine(cx, cy - (int)b, cx, cy + (int)b, color);
            return;
        }

        long long b2 = b * b;
        long long a2 = a * a;

        // --- Región 1 ---
        long long x = 0;
        long long y = b;

        long long d = b * ((4 * b) - (4 * a * a)) + a * a;

        long long stop_x = 2 * b2 * (x + 1);
        long long stop_y = a2 * (2 * y - 1);
        long long stop_x_inc = 2 * b2;
        long long stop_y_inc = 2 * a2;

        while (stop_x < stop_y)
        {
            drawEllipsePoints((int)x, (int)y, cx, cy, color);

            if (d > 0) // SE
            {
                d += (stop_x + b2 - stop_y + a2) << 2;
                y--;
                stop_y -= stop_y_inc;
            }
            else // E
            {
                d += (stop_x + b2) << 2;
            }

            x++;
            stop_x += stop_x_inc;
        }

        // --- Región 2 ---
        d = b2 * (4 * x * x + 4 * x + 1) + a2 * (4 * y * y - 8 * y + 4) - 4 * a2 * b2;

        while (y >= 0)
        {
            drawEllipsePoints((int)x, (int)y, cx, cy, color);

            if (d <= 0) // SE
            {
                d += (stop_x - stop_y + 2 * a2) << 2;
                x++;
                stop_x += stop_x_inc;
            }
            else // S
            {
                d += (-stop_y + 2 * a2) << 2;
            }

            y--;
            stop_y -= stop_y_inc;
        }
    }

    void drawCurrentEllipse(int cx, int cy, long long a, long long b, RGBA color)
    {
        if (m_useOptimizedEllipse)
        {
            drawEllipse2(cx, cy, a, b, color); // Algoritmo Optimizado (Paso 2)
        }
        else
        {
            drawEllipse1(cx, cy, a, b, color); // Algoritmo Original (Paso 1)
        }
    }
    
    void runSimilarityTest()
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << " INICIANDO PRUEBA DE SIMILITUD (10.000 Elipses)" << std::endl;
        std::cout << "========================================" << std::endl;

        m_isTesting = true; // Activamos la intercepción
        bool allMatch = true;
        int errores = 0;

        // Limpiamos la pantalla antes de empezar para ver el renderizado
        std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        for (int i = 0; i < 10000; ++i)
        {
            // 1. Parámetros aleatorios
            // Usamos un margen para que no se salgan demasiado de la pantalla
            int cx = rand() % width;
            int cy = rand() % height;
            int a = rand() % (width / 2);
            int b = rand() % (height / 2);

            // Color aleatorio para que se vea interesante en pantalla
            RGBA color = { (unsigned char)(rand() % 255), (unsigned char)(rand() % 255), (unsigned char)(rand() % 255), 255 };

            // 2. Ejecutar Algoritmo 1 (Original)
            m_capturedPixels.clear(); // Limpiamos buffer de captura
            drawEllipse1(cx, cy, (long long)a, (long long)b, color);
            std::vector<std::pair<int, int>> resultado1 = m_capturedPixels; // Guardamos copia

            // 3. Ejecutar Algoritmo 2 (Optimizado)
            m_capturedPixels.clear(); // Limpiamos buffer de captura
            drawEllipse2(cx, cy, (long long)a, (long long)b, color); // Usamos el método optimizado directo
            std::vector<std::pair<int, int>> resultado2 = m_capturedPixels; // Guardamos copia

            // 4. Comparación RIGUROSA
            bool match = true;

            // A) Comparar cantidad de píxeles (Ni uno más, ni uno menos)
            if (resultado1.size() != resultado2.size())
            {
                match = false;
            }
            else
            {
                // B) Comparar posición exacta de cada píxel
                // Dado que la lógica de recorrido es idéntica, el orden debería ser el mismo.
                for (size_t k = 0; k < resultado1.size(); ++k)
                {
                    if (resultado1[k] != resultado2[k])
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (!match)
            {
                allMatch = false;
                errores++;
                std::cout << "Error en elipse #" << i << " (cx:" << cx << ", cy:" << cy << ", a:" << a << ", b:" << b << ")" << std::endl;
                std::cout << "   Pixels Alg1: " << resultado1.size() << " vs Pixels Alg2: " << resultado2.size() << std::endl;
                // Detenemos al primer error para no saturar la consola
                if (errores > 5) break;
            }

            // Renderizamos visualmente cada 100 elipses para no congelar totalmente la UI visual
            // (Aunque el loop bloqueará el thread principal de todas formas)
        }

        m_isTesting = false; // Desactivamos modo test

        std::cout << "----------------------------------------" << std::endl;
        if (allMatch)
        {
            std::cout << " RESULTADO: EXITOSO" << std::endl;
            std::cout << " Los dos algoritmos generaron EXACTAMENTE los mismos pixeles" << std::endl;
            std::cout << " en las 10.000 pruebas." << std::endl;
        }
        else
        {
            std::cout << " RESULTADO: FALLIDO" << std::endl;
            std::cout << " Se encontraron " << errores << " discrepancias." << std::endl;
        }
        std::cout << "----------------------------------------" << std::endl;

        // Pausa solicitada en el enunciado
        std::cout << "Presione <ENTER> en la consola para continuar..." << std::endl;
        std::cin.get(); // Esto bloqueará la ventana hasta que des Enter en la consola negra
    }

    void runEfficiencyTest(int N)
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << " INICIANDO PRUEBA DE EFICIENCIA (N=" << N << ")" << std::endl;
        std::cout << " Generando datos aleatorios..." << std::endl;

        // 1. Generar N elipses aleatorias y guardarlas en memoria
        // (Es importante generarlas antes para no medir el tiempo del rand())
        std::vector<EllipseData> testData;
        testData.reserve(N); // Optimizamos la memoria del vector

        for (int i = 0; i < N; ++i)
        {
            EllipseData e;
            e.cx = rand() % width;
            e.cy = rand() % height;
            // Radios aleatorios
            e.a = rand() % (width / 2);
            e.b = rand() % (height / 2);
            // Color aleatorio
            e.color = { (unsigned char)(rand() % 255), (unsigned char)(rand() % 255), (unsigned char)(rand() % 255), 255 };
            testData.push_back(e);
        }

        std::cout << " Datos generados. Ejecutando tests..." << std::endl;

        // Aseguramos que no estamos en modo 'captura' (queremos velocidad pura)
        m_isTesting = false;

        // --- TEST ALGORITMO 1 (Original) ---
        // Limpiamos pantalla para empezar fresco (opcional, no afecta tiempo de cómputo significativo)
        // std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 }); 

        double start1 = glfwGetTime();

        for (const auto& el : testData)
        {
            drawEllipse1(el.cx, el.cy, el.a, el.b, el.color);
        }

        double end1 = glfwGetTime();
        double time1 = (end1 - start1) * 1000.0; // Convertimos a milisegundos (ms)

        // --- TEST ALGORITMO 2 (Optimizado) ---
        // Limpiamos pantalla
        // std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        double start2 = glfwGetTime();

        for (const auto& el : testData)
        {
            drawEllipse2(el.cx, el.cy, el.a, el.b, el.color);
        }

        double end2 = glfwGetTime();
        double time2 = (end2 - start2) * 1000.0; // Convertimos a milisegundos (ms)

        // --- RESULTADOS ---
        std::cout << "----------------------------------------" << std::endl;
        std::cout << " RESULTADOS DE EFICIENCIA (N=" << N << ")" << std::endl;
        std::cout << " Algoritmo 1 (Original):   " << time1 << " ms" << std::endl;
        std::cout << " Algoritmo 2 (Optimizado): " << time2 << " ms" << std::endl;

        double diff = time1 - time2;
        double percent = (diff / time1) * 100.0;

        if (time2 < time1)
        {
            std::cout << " >> MEJORA: El optimizado es " << percent << "% mas rapido." << std::endl;
        }
        else
        {
            std::cout << " >> RESULTADO: El optimizado es " << -percent << "% mas lento (o igual)." << std::endl;
        }

        std::cout << "----------------------------------------" << std::endl;
    }

    void update()
    {
        // Limpiamos el buffer a color negro transparente
        std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        //Dibujamos todas las l�neas guardadas en el vector
        for (const LineData& line : m_lines)
        {
            if (m_useRealArithmetic)
                drawLineReal(line.x0, line.y0, line.x1, line.y1, line.color);
            else
                drawLine(line.x0, line.y0, line.x1, line.y1, line.color);
        }

        // Dibujamos la l�nea actual
        if (m_useRealArithmetic)
            drawLineReal(m_x0, m_y0, m_x1, m_y1, m_current_color);
        else
            drawLine(m_x0, m_y0, m_x1, m_y1, m_current_color);

        // Dibujamos todas las elipses guardadas en el vector
        for (const EllipseData& ellipse : m_ellipses)
        {
            drawCurrentEllipse(ellipse.cx, ellipse.cy, ellipse.a, ellipse.b, ellipse.color);
        }

        // Dibujamos la elipse interactiva S�LO si estamos en modo Elipse
        // Y ADEM�S el bot�n izquierdo del mouse est� presionado.
        if (mouseButtonsDown[GLFW_MOUSE_BUTTON_LEFT] && m_drawingMode == EDrawingMode::Ellipse)
        {
            drawCurrentEllipse(m_cx, m_cy, (long long)m_a, (long long)m_b, m_current_color);
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
        ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_Once);
        ImGui::Begin("Controles Tarea 1 y 2");

        // --- A�ADIR ESTO: SELECCI�N DE MODO ---
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
        // --- FIN DE A�ADIDOS ---
        ImGui::Separator();

        // --- SECCIÓN TAREA 2 (NUEVA) ---
        ImGui::Text("Algoritmo Elipse (Tarea 2)");
        // Este checkbox controla la variable m_useOptimizedEllipse
        ImGui::Checkbox("Usar Optimizado (drawEllipse2)", &m_useOptimizedEllipse);

        ImGui::Spacing();
        
        // --- AÑADIR ESTO ---
        if (ImGui::Button("Ejecutar Prueba Similitud (10k)"))
        {
            // Llamamos a la función de prueba
            runSimilarityTest();
        }
        // --- FIN DE AÑADIDO ---

        ImGui::Separator();
        // --- AÑADIDO PARA PRUEBA DE EFICIENCIA (PASO 4) ---
        ImGui::Text("Pruebas de Eficiencia (Tiempos)");

        if (ImGui::Button("Test 50.000"))
        {
            runEfficiencyTest(50000);
        }
        ImGui::SameLine();
        if (ImGui::Button("Test 100.000"))
        {
            runEfficiencyTest(100000);
        }

        if (ImGui::Button("Test 500.000"))
        {
            runEfficiencyTest(500000);
        }
        ImGui::SameLine();
        if (ImGui::Button("Test 1.000.000")) // ¡La prueba de fuego!
        {
            runEfficiencyTest(1000000);
        }

        ImGui::Text("Controles Tarea 1 - L�neas");
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
                    255                           // A (s�lido)
                };

                // A�adimos la l�nea al vector
                m_lines.push_back({ x0, y0, x1, y1, rand_color });
            }
            std::cout << "A�adidas 1000 lineas aleatorias. Total: " << m_lines.size() << std::endl;
        }

        ImGui::SameLine(); // Pone el siguiente widget en la misma l�nea

        if (ImGui::Button("Limpiar Todo"))
        {
            m_lines.clear();
			m_ellipses.clear();
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
            //std::cout << "Tecla " << key << " presionada\n";
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
        else if (action == GLFW_RELEASE)
            std::cout << "Tecla " << key << " soltada\n";
    }

    void onMouseButton(int button, int action, int mods)
    {
        ImGuiIO& io = ImGui::GetIO();
        // Si estamos sobre un widget, no hacemos nada. Evita creaci�n de l�neas al interactuar con el men�
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

                // --- L�GICA DE MODO ---
                if (m_drawingMode == EDrawingMode::Line)
                {
                    // Modo L�nea: Inicia la l�nea
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
                // --- FIN L�GICA DE MODO ---
            }
            else if (action == GLFW_RELEASE)
            {
                mouseButtonsDown[button] = false;
                std::cout << "Mouse soltado en (" << xpos << ", " << ypos << ") -> Renderizado en (" << (int)xpos << ", " << render_y << ")\n";

                // --- L�GICA DE MODO ---
                if (m_drawingMode == EDrawingMode::Line)
                {
                    // Modo L�nea: Finaliza y guarda la l�nea
                    m_x1 = (int)xpos;
                    m_y1 = render_y;

                    LineData finished_line = { m_x0, m_y0, m_x1, m_y1, m_current_color };
                    m_lines.push_back(finished_line);
                    std::cout << "Linea guardada. Total de lineas: " << m_lines.size() << std::endl;

                    // Reseteamos la l�nea interactiva
                    m_x0 = -1; m_y0 = -1; m_x1 = -1; m_y1 = -1;
                }
                else if (m_drawingMode == EDrawingMode::Ellipse)
                {
                    // Modo Elipse: Finaliza y guarda la elipse
                    // Los radios son la distancia absoluta desde el centro
                    m_a = (int)std::abs(xpos - m_cx);
                    m_b = (int)std::abs(render_y - m_cy);

                   EllipseData finished_ellipse = { m_cx, m_cy, (long long)m_a, (long long)m_b, m_current_color };
                   m_ellipses.push_back(finished_ellipse);
				   std::cout << "Elipse nueva guardada en (" << m_cx << ", " << m_cy << ") con a=" << m_a << ", b=" << m_b << std::endl;
                   std::cout << "Elipse guardada. Total de elipses: " << m_ellipses.size() << std::endl;
                }
                // --- FIN L�GICA DE MODO ---
            }
        }
    }

    void onCursorPos(double xpos, double ypos)
    {
        ImGuiIO& io = ImGui::GetIO();
        // Si estamos sobre un widget, no hacemos nada. Evita creaci�n de l�neas al interactuar con el men�
        if (io.WantCaptureMouse)
            return;

        if (mouseButtonsDown[GLFW_MOUSE_BUTTON_LEFT])
        {
            int render_y = height - 1 - (int)ypos; // Y invertida

            // --- L�GICA DE MODO ---
            if (m_drawingMode == EDrawingMode::Line)
            {
                // Modo L�nea: actualiza el punto final
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
