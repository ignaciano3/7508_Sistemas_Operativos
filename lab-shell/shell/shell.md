# Lab: shell

### Búsqueda en $PATH

- ***¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?***

    Las diferencias entre la syscall `execve` y las funciones wrappers de la misma son:
    - Los parametros que recibe.
    - El formato en dichos parametros.

    Las variantes pueden ser identificadas por medio del nombre de las funciones:

    - Si la función contiene una `l` en su nombre, la misma recibe parametros separados por cada argumento correspondiente
    al argv que vaya a tener el archivo a ser ejecutado.
    En caso contrario la funcion contiene una `v` en el nombre, y recibe un solo vector de punteros a strings, como si se
    tratase directamente de argv.

    - Si la función contiene una `p` en su nombre, la misma utiliza la misma logica que la shell para buscar el archivo binario
    en PATH correspondiente al nombre que se le pase por parametro. 
    En caso de no contenerla, recibe el path completo al binario requerido para ejecutar.

    - Si la funcion contiene una `e` en su nombre, la misma recibe un vector de punteros a strings conteniendo las variables de entorno
    (en formato "CLAVE=VALOR") que se requieren tener en el binario a ejecutar.
    En caso contrario el binario a ser ejecutado hereda las variables de entorno del proceso actual, por medio del uso de la variable
    `environ`. 

- ***¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?***

    La llamada puede fallar por diversos motivos. Un ejemplo basico sería falla en encontrar el binario especificado.
    En la implementación actual se realiza la verificación del valor de retorno en caso de falla (ya que si exec no falla nunca
    retorna pues se "reemplaza" el proceso sobre sí mismo) y se utiliza perror para mostrar la causa de falla de la syscall execve
    usada dentro de la funcion wrapper correspondiente.

---

### Comandos built-in

- ***¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)***

    `cd` debe ser built-in y no programa aparte, ya que se espera que se cambie el directorio en el que se
    encuentra la shell, por lo cual debería efectuar el cambio sobre ese mismo proceso padre.

    Podría implementarse `pwd` como programa aparte, dado que se posee la función getcwd(3) para obtener el path actual en el que se 
    encuentre trabajando la shell. Sin embargo resulta más conveniente su implementación como built-in debido a que se pueden
    ahorrar llamados a syscalls como fork, execve y wait (lo cual ahorra cambios de contexto, reduciendo en gran medida el
    costo de la operación, que en principio se desearía poca).

---

### Variables de entorno adicionales

- ***¿Por qué es necesario hacerlo luego de la llamada a fork(2)?***

    Esto se debe a que las variables de entorno adicionales temporales deben permanecer exclusivamente en el entorno del proceso
    hijo, y por ende solo durante su ejecución. No deberían ser mantenidas como valores del entorno de la shell, lo cual sucede
    si se realiza el set de las variables previo al fork.

- ***En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de exec(3). ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué. Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.***

    El comportamiento sigue siendo el mismo, siempre y cuando se adquieran manualmente todas las variables de entorno que serían
    heredadas en el caso de usar un wrapper SIN terminación `e`. Esto podria realizarse a través de la obtencion de todos los
    pares `CLAVE=VALOR` de la variable global `environ` (cuya documentación detallada está en el manual de linux).
    A partir de tener un vector con todos los valores, se pueden añadir los valores necesarios que se requieran como variables
    de entorno temporales adicionales. 

---

### Procesos en segundo plano

- ***Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano***

    La implementación consiste en:

    Posterior al llamado a fork, en el proceso hijo se llama recursivamente a la función `exec_cmd`. El parámetro es un `struct cmd`, el 
    cual es nuestra abstraccion de comando generico para permitir comportamiento compartido como si se tratase de una interfaz.

    Teniendo en cuenta que se implementa la ejecución de procesos en segundo plano, se espera que sea posible seguir ejecutando
    distintos procesos que sea necesario esperar de forma inmediata que el anteriormente ejecutado acabe. 
    
    Para ello se utiliza, en el proceso padre del fork, la syscall `waitpid` con la flag `WNOHANG`, que permite un comportamiento no bloqueante en la espera de procesos
    hijos, permitiendo continuar con la ejecución de la shell. Además se utiliza `WAIT_ANY` para esperar a cualquier proceso hijo que
    haya quedado en segundo plano anteriormente. Este primer llamado a `waitpid` se realiza siempre, ya que justamente a pesar de que se
    ponga en segundo plano se va a querer esperar a cualquier hijo que previamente se haya puesto en tal estado.

    Luego si el comando parseado no requiere estar en segundo plano se realiza el otro `waitpid` (distinto), que espera de forma
    bloqueante como siempre.

---

### Flujo estándar

- ***Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?***

    `2>&1` consiste en redirigir los contenidos del file descriptor 2 (stderr) hacia lo que esté apuntando el file descriptor 1 (es
    decir, no necesariamente stdout, ya que si el file descriptor 1 fue redirigido a otro destino entonces esta redirección también
    llegará al mismo paradero).

    En el ejemplo `>out.txt 2>&1` primero se redirige la salida del fd 1 (que era `stdout`) hacia el archivo `out.txt`.
    Luego al ejecutar `ls` de un archivo o dir que no existe va a escribir el error en el fd 2 (`stderr`) pero este fue redirigido hacia &1, entonces tanto la salida que iría a `stdout` como el error se van a guardar en `out.txt`.

    `Ejemplo anterior:`
    ```bash
    $ ls -C /home /noexiste >out.txt 2>&1
    $ cat out.txt
    ls: cannot access '/noexiste': No such file or directory
    /home:
    username
    ```

    Si si invierte el orden como `2>&1 >out1.txt`, primero se va a escribir el error por `stdout` (por la redirección `2>&1`), ya que hasta ese punto no se realizó la redirección `>out.txt`. 
    Recién al final se realiza esa redirección, por lo cual la salida estandar de `ls` si se va a guardar en el `out.txt` y no se va a mostrar en `stdout`.

    `Ejemplo anterior:`
    ```bash
    $ ls -C /home /noexiste 2>&1 >out1.txt
    ls: cannot access '/noexiste': No such file or directory
    $ cat out1.txt
    /home:
    miguelv5
    ```

---

### Tuberías simples (pipes)

- ***Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab***

    a

---

### Pseudo-variables

- ***Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar)***

    - `$0` se reemplaza por el nombre del archivo que actualmente se está ejecutando como script
    ```bash
    $ echo $0
    bash

    $ cat ejemplo2.sh 
    echo "Nombre deberia ser ./ejemplo2.sh. Resultado:  $0"
    $ ./ejemplo2.sh
    "Nombre deberia ser ./ejemplo2.sh. Resultado:  ./ejemplo2.sh"

    ```
    
    - `$$` se reemplaza por el PID de la shell que está siendo ejecutada.
    ```bash
    $ echo $$
    8321
    ```

    - `$!` se reemplaza por el PID del último proceso ejecutado en segundo plano
    ```bash
     $ sleep 2 &
    [1] 70180
    $ 
    [1]  + done       sleep 2
    $ echo $!
    70180
    ```


---

