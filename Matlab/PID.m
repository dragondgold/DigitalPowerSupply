s = tf('s');

R = 10;         % Resistencia de carga [Ohm]
Rc = 0.35;      % ESR del capacitor de salida [Ohm]
C = 470e-6;     % Capacidad de salida
Rl = 118e-3;    % Resistencia del inductor
L = 100e-6;     % Inductancia
Vi = 30;        % Tensión de entrada al buck

% Función de transferencia del Convertidor Buck
H = Vi * ( (R + (s*R*Rc*C))/((s^2) * (L*C*(R+Rc)) + s*(R*Rc*C + Rl*C*(R+Rc) + L) + R + Rl) )

% Iniciamos el asistente para calcular los parametros del PID
pidTuner(H, 'PID')
