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

    NO es el mismo comportamiento tal cual como se menciona, dado que las funciones wrappers con la terminación `e` tienen que ser provistas por
    un arreglo que contenga TODAS las variables de entorno incluyendo las que se heredarían normalmente del proceso padre a partir de sus propias
    variables. Mientras que el uso de `setenv` para añadir variables de entorno nuevas solo requiere, justamante, del seteo de aquellas variables
    nuevas a adicionar.
    Sin embargo puede llegar a obtenerse el mismo comportamiento de la siguiente manera:
    Se adquieren manualmente todas las variables de entorno que serían heredadas en el caso de usar un wrapper SIN terminación `e`
    (Esto puede realizarse a través de la obtencion de todos los pares `CLAVE=VALOR` de la variable global `environ` cuya documentación detallada está en
    las paginas de manual de linux).
    A partir de dicha obtencion, se puede construir un vector con todos los valores que deberían ser heredados, y a partir del mismo se le pueden añadir los valores necesarios que se requieran como variables de entorno temporales adicionales.
    Una vez hecho esto se puede realizar el llamado a la funcion wrapper deseada con dicho vector como ultimo parametro.

---

### Procesos en segundo plano

- ***Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano***

    La implementación consiste en:

    Posterior al llamado a fork, en el proceso hijo se llama recursivamente a la función `exec_cmd`. 

    Teniendo en cuenta que se implementa la ejecución de procesos en segundo plano, se espera que sea posible seguir ejecutando
    distintos procesos sin que sea necesario esperar de forma inmediata que el anteriormente ejecutado acabe. 
    
    Para ello se utiliza, en el proceso padre del fork, la syscall `waitpid` con la flag `WNOHANG`, que permite un comportamiento no bloqueante en la espera de procesos hijos, permitiendo continuar con la ejecución de la shell. Además se utiliza `WAIT_ANY` para esperar a cualquier proceso hijo que haya quedado en segundo plano anteriormente. Este primer llamado a `waitpid` se realiza siempre, ya que justamente a pesar de que se ponga en segundo plano se va a querer esperar a cualquier hijo que previamente se haya puesto en tal estado.

    Luego si el comando parseado no requiere estar en segundo plano se realiza el otro `waitpid` (distinto), que espera de forma
    bloqueante. Como este ultimo llamado espera al comando especifico (que no es BACK) recien ejecutado por medio de su PID,
    los llamados a waitpid no interfieren entre sí, siempre esperan objetivos distintos.

---

### Flujo estándar

- ***Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?***

    `2>&1` consiste en redirigir los contenidos del file descriptor 2 (stderr) hacia lo que esté apuntando el file descriptor 1 (es
    decir, no necesariamente stdout, ya que si el file descriptor 1 fue redirigido a otro destino entonces esta redirección también
    llegará al mismo paradero).

    En el ejemplo `>out.txt 2>&1` (implementacion de bash) primero se redirige la salida del fd 1 (que era `stdout`) hacia el archivo `out.txt`.
    Luego al ejecutar `ls` de un archivo o dir que no existe va a escribir el error en el fd 2 (`stderr`) pero este fue redirigido hacia &1, entonces tanto la salida que iría a `stdout` como el error se van a guardar en `out.txt`.
    En la implementación actual se obtiene el mismo comportamiento, conservando ese mismo orden.

    `Ejemplo anterior, tanto en bash como en la implementación realizada:`
    ```bash
    $ ls -C /home /noexiste >out.txt 2>&1
    $ cat out.txt
    ls: cannot access '/noexiste': No such file or directory
    /home:
    miguelv5
    ```

    Si si invierte el orden como `2>&1 >out1.txt`, el comportamiento (en la implementación de bash) es que primero se va a escribir el error por `stdout` (por la redirección `2>&1`), ya que hasta ese punto no se realizó la redirección `>out.txt`, entonces el file descriptor 1 aún correspondía a `stdout`. 
    Recién al final se realiza esa redirección, por lo cual la salida estandar de `ls` si se va a guardar en el `out.txt` y no se va a mostrar en `stdout`.
    Este comportamiento **difiere** de la implementación actual, ya que en la misma no se verifica que el orden de las redirecciones
    sean adaptables a su posición en el comando, siendo que bash lo interpreta de izquierda a derecha.
    Teniendo lo anterior en cuenta, la implementación realizada se comporta como en el primer ejemplo mostrado, y no es afectada por
    el orden de las redirecciones en el comando.  

    `Ejemplo anterior en bash:`
    ```bash
    $ ls -C /home /noexiste 2>&1 >out1.txt
    ls: cannot access '/noexiste': No such file or directory
    $ cat out1.txt
    /home:
    miguelv5
    ```

    `Ejemplo anterior en la implementación realizada:`
    ```bash
    $ ls -C /home /noexiste 2>&1 >out1.txt
    $ cat out.txt
    ls: cannot access '/noexiste': No such file or directory
    /home:
    miguelv5
    ```

---

### Tuberías simples (pipes)

- ***Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab***

    - Si todos los comandos (tanto en bash como en la implementación realizada) fueron exitosos, el exit code es 0.
    ```bash
    $ ls -C $HOME | grep Doc | wc | xargs -n 1 echo hola
    hola 1
    hola 6
    hola 67
    $ echo $?
    0
    ```
    
    - Sin embargo, si alguno de los comandos falla difiere el comportamiento de las implementaciones:
    
    `En bash, los comandos respetan un orden de izquierda a derecha aun cuando falla alguno de los comandos. Por ende SOLO si falla el ultimo comando de la derecha se va a tener un exit code de error. En el resto de casos (donde falla un comando intermedio o el de la izquierda del todo) se van a ejecutar todos los comandos normalmente a partir del siguiente al que tuvo la falla:`
    ```bash

    $ ls -C $HOME | grep Doc | wc | xargs_fail -n 1 echo hola
    xargs_fail: command not found
    $ echo $?
    127


    $ ls -C $HOME | grep Doc | wc_fail | xargs -n 1 echo hola
    wc_fail: command not found
    hola
    $ echo $?
    0


    $ ls -C $HOME | grep_fail Doc | wc | xargs -n 1 echo hola
    grep_fail: command not found
    hola 0
    hola 0
    hola 0
    $ echo $?
    0


    $ ls_fail -C $HOME | grep Doc | wc | xargs -n 1 echo hola
    ls_fail: command not found
    hola 0
    hola 0
    hola 0
    $ echo $?
    0

    ```

    `En la implementación realizada TAMBIEN se mantiene ese orden de ejecución, esto debido a cómo se realizan los forks y se mantiene manipulado el orden por medio de el uso de waits. Sin embargo no se mantienen los exit status debido a como está implementado printstatus.c (ignora los comandos de tipo pipecmd), por lo tanto aunque cualquiera de los comandos falle no se puede observar el exit status de error:`
    ```bash

    (/home/miguelv5) 
    $ ls -C $HOME | grep Doc | wc | xargs -n 1 echo hola
    hola 1
    hola 6
    hola 67
     (/home/miguelv5) 
    $ echo $?
    0
    	... 


     (/home/miguelv5) 
    $ ls -C $HOME | grep Doc | wc | xargs_fail -n 1 echo hola
    Error en syscall [execve]
    No such file or directory
     (/home/miguelv5) 
    $ echo $?  
    0
    	... 


     (/home/miguelv5)
    $ ls -C $HOME | grep Doc | wc_fail | xargs -n 1 echo hola
    Error en syscall [execve]
    No such file or directory
    hola
    (/home/miguelv5) 
    $ echo $?
    0
        ... 


    (/home/miguelv5) 
    $ ls -C $HOME | grep_fail Doc | wc | xargs -n 1 echo hola
    Error en syscall [execve]
    No such file or directory
    hola 0
    hola 0
    hola 0
    (/home/miguelv5) 
    $ echo $?
    0
        ... 


    (/home/miguelv5) 
    $ ls_fail -C $HOME | grep Doc | wc | xargs -n 1 echo hola
    Error en syscall [execve]
    No such file or directory
    hola 0
    hola 0
    hola 0
    (/home/miguelv5) 
    $ echo $?
    0

    ```

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

