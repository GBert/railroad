# Configuración de los desvíos
En la pantalla principal se puede ir a la configuración de los desvíos con el icono ![](../menu_switch.png).

## Datos básicos
![](switches_basics_es.png)

### Nombre
Cada desvío necesita un nombre inequívoco. Si el nombre no está indicado RailControl crea un nombre. Si el nombre ya existe RailControl añade un numero al nombre para hacer el nombre inequívoco.

### Tipo
RailControl conoce varios tipos de desvíos:

izquierda: En el diagrama de vías se muestra el elemento como un desvío izquierda. El simbolo no necesita que ser igual como el tipo de desvío en la maqueta.

derecha: En el diagrama de vías se muestra el elemento como un desvío derecha. El simbolo no necesita que ser igual como el tipo de desvío en la maqueta.

tres vías: Este tipo representa dos desvíos al mismo lugar. Uno de los cuatro posiciones logicos no existe en la realidad. RailControl no permite esta posición. El desvío izquierda está controlado por la dirección seleccionada, el desvío derecha está controlado por la dirección siguiente.

Märklin desvío de cruce doble izquierda: Los desvíos de cruce doble de Märklin solamente requieren un motor.

Märklin desvío de cruce doble derecha: Los desvíos de cruce doble de Märklin solamente requieren un motor.

Se puede configurar los desvíos de cruce doble con dos motores como dos desvíos simples. Importante es que el motor fisico a la izquierda es controlado por el desvío a la derecha en RailControl.

### Control
Si hay más que un control configurado en RailControl, se tiene que seleccionar el control que controla el desvío. Si solamente un control está configurado en RailControl el campo de selección no está visible.

### Protocolo
Si un control soporta más que un protocolo, se tiene que seleccionar el protocolo que controla el desvío. Si el control solamente soporta un protocolo, el campo de selección no está visible.

### Dirección
La dirección que controla el desvío.

### Duración de conmutación (ms)
Después del procedimiento de conmutación se tiene que apagar los accesorios. Se puede apagar los nuevos accesorios después de 100ms. A veces accesorios más viejos o lentos necesitan 250ms. Algunos controles apagan los accesorios automáticamente con un valor que se puede configurar directamente por el control. En este caso se puede configurar 0ms aquí. 

### Invertido
Cuando los conectores del accesorio están conectado invertido, RailControl puede invertir otra vez.

## Position
![](switches_position_es.png)

### Posición X
La posición del elemento en cuadros deste la izquierda en el diagrama de vías. Se empieza a contar con zero. Si un elemento es más grande que un cuadrado el cuadrado izquierda arriba es importante para contar.

### Posición Y
La posición del elemento en cuadros deste arriba en el diagrama de vías. Se empieza a contar con zero. Si un elemento es más grande que un cuadrado el cuadrado izquierda arriba es importante para contar.

### Capa
La capa en que el elemento está visible.

### Rotación
Se puede rotar los elementos en pasos de 90 grados.

