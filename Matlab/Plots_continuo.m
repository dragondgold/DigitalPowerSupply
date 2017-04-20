% Parametros calculados del PID
Kp = 0.175
Ki = 371.22
Kd = 0

% Creamos PID
G = pid(Kp,Ki,Kd)

% Creamos el sistema de lazo cerrado siendo G la planta
T_pid = feedback(G*H, 1)

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
step(T_pid, 0:10e-6:1e-3)
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
%title('Respuesta en frecuencia en lado cerrado (plano continuo)')
%xlabel('Frecuencia')
%ylabel('Amplitud')

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