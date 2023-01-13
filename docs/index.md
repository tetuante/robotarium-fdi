# IX Semana de la Informática. Concurso/Taller Robotarium - FDI

Bienvenidos al taller *Robotarium - FDI* y muchas gracias por colaborar en los primeros pasos de nuestro futuro *robotarium*.

Recuerda que si quieres obtener ECTS por participar en actividades de la Seamana de la Informática,
[debes inscribire en este formulario](https://web.fdi.ucm.es/ActividadesFormativas/). si no lo has hecho ya.

 

## Estructura del Taller

Este taller se compone de varias fases:
 
* `Construcción del robot` - En equipos de 3 personas, construiréis el robot y haréis que mueva sus ruedas.
* `Recorrer una distancia` - Programaréis el robot para que recorra una distancia predefinida.
* `Hacer un círculo` - Programaréis el robot para que recorra un círculo de 1m de radio. 
* `Hacer un cuadrado` - Programaréis el robot para que recorra un cuadrado. ¡Cuidado con las esquinas!

## Montaje del Robot

### Montar la chapa roja

Esto es un ejmplo de inclusión de una figura (puede ser .JPG, .PNG...)

![ejemplo figura](img/robot.jpg)

## Ejemplo de código

```c
if (a==b)
    turn_right();
else
    turn_left();
```

## Cuadros de colores

!!! note "Cuestión"
    * ¿Qué componente se está incluyendo además de los que siempre se incluyen por defecto?
    * ¿Qué funcionalidad se importa de dicho componente?


!!! danger "Tareas"
    * Crea dos nuevos componentes en el proyecto creado a partir del ejemplo. Uno de ellos tendrá al menos una función `int get_hall_read()` que proporcionará una lectura del sensor de efecto Hall. El otro tendrá al menos una función `int get_temperature()` que deolverá la temperatura medida en grados Celsius obtenida del sensor Si7021. Los componentes podrán tener más funciones, tanto públicas como privadas. Antes de comenzar el REPL (que comienza con la llamada `esp_console_start_repl()`) se deberá mostrar por terminal una lectura de cada sensor.

