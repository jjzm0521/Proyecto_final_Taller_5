# Backlog de Tareas: Proyecto "Misión Aterrizaje" STM32

🛠️ Fase 0: Motor Gráfico (Prioridad Alta)El objetivo es tener una pantalla que actualice rápido y sin parpadeos.
[X] Optimización setAddrWindow
 [X] Revisar la función actual para asegurarse de que envía los comandos 0x2A y 0x2B correctamente.
 [X] Eliminar retardos innecesarios dentro de esta función.
[X] Implementar fillRect Relleno Rápido( ) Crear función que acepte (x, y, w, h, color).
 [X] Configurar la ventana de dirección una sola vez al inicio.
 [X] Hacer un bucle que envíe solo los datos de color w * h veces (sin volver a enviar coordenadas).
[X] Implementar drawBitmap (Sprites)
 [X] Crear función para recibir un array const uint16_t imagen[].
 [X] Probar dibujando un icono de 16x16 o 32x32 píxeles en el centro.
[X] Sistema de "Borrado Inteligente"
 [X] Crear lógica para borrar el cohete: dibujar un rectángulo negro (fillRect) solo en la posición anterior de la nave antes de dibujar la nueva.
 [X] Verificar FPS visualmente (el movimiento debe verse fluido).

📡 Fase 1: Drivers de SensoresLectura del mundo real.
[X] Driver I2C (GY-521 / MPU6050)
 [X] Configurar pines PB6 (SCL) y PB7 (SDA) en modo Alternate Function Open-Drain.
 [X] Escribir función de inicio: despertar el sensor (escribir 0 en registro PWR_MGMT_1).
 [X] Crear función I2C_Read(devAddr, regAddr, *data, length).
[X] Matemática del Acelerómetro
 [X] Leer registros de aceleración X, Y, Z.
 [ ] Calcular ángulo de inclinación (Pitch/Roll) usando atan2.
 [ ] Implementar filtro simple: angulo = 0.9* angulo_anterior + 0.1 * lectura_actual.
[X] Driver ADC (Joystick)
 [X] Configurar PA0 como entrada analógica.
 [X] Configurar ADC1 para lectura continua o por solicitud.
 [X] Crear función getThrust() que retorne un float de 0.0 a 1.0 basado en la lectura (0 a 4095).

🚀 Fase 2: Física y Lógica del JuegoEl cerebro del simulador.
 [X] Estructura de Datos
 [X] Definir struct Nave { float x, y; float vx, vy; float combustible; }.
[X] Inicializar la nave en y = 10 (arriba) y vy = 0.
 [X] Gravedad y Movimiento
 [X] Crear timer o delay para el loop de física (ej. cada 50ms).
 [X] Aplicar gravedad: nave.vy += 0.05 (ajustar valor experimentalmente).
[X] Actualizar posición: nave.y += nave.vy.
 [X] Integración de Empuje
 [X] Si Joystick > Umbral: nave.vy -= empuje * cos(angulo).
[ ] Restar combustible al usar empuje.
 [X] Detección de Colisión Suelo( ) Añadir if (nave.y >= SUELO_Y).
 [ ] Dentro del if, chequear velocidad: Si nave.vy > VEL_MAX_ATERRIZAJE -> Estado EXPLOSION.
 [ ] Si nave.vy <= VEL_MAX -> Estado VICTORIA.

🔊 Fase 3: Sonido y UARTFeedback para el usuario.
[ ] Sistema de Audio (PWM)
 [ ] Configurar Timer (ej. TIM2 o TIM3) en modo PWM Output.
 [ ] Crear función setBuzzerFreq(hz).
 [ ] Lógica: Si hay empuje, freq = 100Hz. Si explota, variar frecuencia aleatoriamente (ruido).
[ ] Comunicación UART
 [ ] Configurar USART2 (Tx/Rx) a 9600 o 115200 baudios.
 [ ] Redirigir printf o crear función UART_SendString.
 [ ] Formatear datos para Plotter: enviar "Angulo, VelocidadY, Altura\n".

🎨 Fase 4: Pulido y UIHacer que parezca un juego real.
[ ] Máquina de Estados Principal
 [ ] Implementar switch (gameState) en el main.
 [ ] Casos: MENU, GAME, RESULT_SCREEN.
[ ] Pantalla de Inicio
 [ ] Dibujar texto "PRESIONE START" (usar botón del joystick o acelerar a fondo).
[ ] Gráficos del Terreno
 [ ] Dibujar línea de suelo.
 [ ] Dibujar "Pad" de aterrizaje (rectángulo verde) en una zona específica.
 [ ] (Opcional) Detectar si la nave cae DENTRO del pad horizontalmente.
[ ] HUD (Heads Up Display)
 [ ] Dibujar barra de combustible (rectángulo que se encoge).
 [ ] Mostrar velocidad vertical numérica en una esquina.
