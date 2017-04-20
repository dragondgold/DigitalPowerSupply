s = tf('s');

R = 10;         % Resistencia de carga, consideramos una corriente de 1.5A
Rc = 130*10e-3; % ESR de los 3 capacitores en paralelo a la salida
C = 1410e-6;    % Capacidad de salida
Rl = 100e-3;    % Resistencia del inductor
L = 500e-6;     % Inductancia
Vi = 30;        % Tensión de entrada al buck

% Función de transferencia del Convertidor Buck
H = Vi * ( (R + (s*R*Rc*C))/((s^2) * (L*C*(R+Rc)) + s*(R*Rc*C + Rl*C*(R+Rc) + L) + R + Rl) )

% Iniciamos el asistente para calcular los parametros del PID
pidTuner(H, 'PID')
