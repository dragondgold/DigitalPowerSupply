% Parametros calculados del PID
Kp = 1.7
Ki = 18000
Kd = 5e-6

% Creamos PID
G = pid(Kp,Ki,Kd)

% Creamos el sistema de lazo cerrado siendo G la planta
T_pid = feedback(G*H, 1)
Fc = bandwidth(T_pid)/(2*pi)

% Graficamos la respuesta del sistema con lazo cerrado
figure(1);
step(T_pid)
grid on
title('Respuesta transitoria en lazo cerrado (plano continuo)')
xlabel('Tiempo')
ylabel('Amplitud')

% Graficamos la respuesta del sistema con lazo cerrado observando un
%  tiempo mayor para ver la respuesta en estado estacionario
figure(2);
step(T_pid, 0:10e-6:10e-3)
grid on
title('Estado estacionario en lazo cerrado (plano continuo)')
xlabel('Tiempo')
ylabel('Amplitud')

% Graficamos la respuesta en frecuencia con lazo cerrado
optsV = bodeoptions('cstprefs');
optsV.FreqUnits = 'Hz';
optsV.Grid = 'on';

figure(3);
bode(T_pid, optsV)
grid on

% Diagrama de polos y ceros con lazo cerrado
[p, z] = pzmap(T_pid)   % Obtenemos los valores de los polos y ceros
figure(4);
pzmap(T_pid)
grid on
title('Diagrama de polos y ceros en lazo cerrado (plano continuo)')
axis equal

% Diagrama de Nyquist
figure(5)
nyquist(T_pid)
grid on
title('Diagrama de Nyquist en lazo cerrado (plano continuo)')
axis equal