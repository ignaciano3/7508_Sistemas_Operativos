#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <errno.h>

#include <sys/mman.h>

#include "malloc.h"
#include "printfmt.h"

#ifdef USE_STATS
#include "statistics.h"
extern stats_t stats;
#define INCREASE_STATS(stat_name, amnt)                                        \
	if ((int) (amnt) > 0)                                                  \
	stats.stat_name += amnt
#define DECREASE_STATS(stat_name, amnt) (stats.stat_name -= amnt)
#else
#define INCREASE_STATS(stat_name, amnt)
#define DECREASE_STATS(stat_name, amnt)
#endif

#define MAGIC_32BIT 0xBE5A74CDU

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) (r + 1)
#define PTR2REGION(ptr) ((region_header_t *) ptr - 1)

#define BLOCK2REGION(ptr) ((region_header_t *) (ptr + 1))
#define REGION2BLOCK(ptr) ((block_header_t *) ptr - 1)

#define SPLITREGION(ptr, size)                                                 \
	(region_header_t *) ((byte *) REGION2PTR(ptr) + size)

#define KiB 1024
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)

#define MIN_REGION_LEN 64

#define BLOCK_SM (16 * KiB)
#define BLOCK_MD (1 * MiB)
#define BLOCK_LG (32 * MiB)

#define HARD_LIMIT (10 * BLOCK_LG)

#define EXIT_OK 0
#define EXIT_ERROR -1
#define INVALID_POINTER 134

//=========================================================//
//======================== STRUCTS ========================//
//=========================================================//

typedef char byte;

typedef struct block_header {
	struct block_header *next;
	struct block_header *prev;
	int size;
} block_header_t;

typedef struct region_header {
	struct region_header *next;
	struct region_header *prev;
	size_t size;
	bool free;
	unsigned int check_num;
} region_header_t;

block_header_t *block_header_list = NULL;
block_header_t *block_header_tail = NULL;
size_t total_mmy = 0;

//===========================================================//
//======================== SIGNATURE ========================//
//===========================================================//

/// @brief Busca una nueva region libre mediante la técnica de First Fit.
/// @param start_region Región de comienzo.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Retorna la región encontrada o nulo en caso de que no hayan regiones que se ajusten a lo solicitado.
region_header_t *loop_for_first_fit_region(region_header_t *start_region,
                                           size_t size);

/// @brief Busca una nueva region libre mediante la técnica de First Fit a lo
/// largo de todos los bloques.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Retorna la región encontrada o nulo en caso de que no hayan regiones que se ajusten a lo solicitado.
static region_header_t *find_region_first_fit(size_t size);

/// @brief Busca una nueva region libre mediante la técnica de Best Fit.
/// @param start_region Región de comienzo.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @param curr_best_region Mejor región actual.
/// @return Retorna la región encontrada o nulo en caso de que no hayan regiones que se ajusten a lo solicitado.
region_header_t *loop_for_best_fit_region(region_header_t *start_region,
                                          size_t size,
                                          region_header_t *curr_best_region);

/// @brief Busca una nueva region libre mediante la técnica de Best Fit a lo
/// largo de todos los bloques.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Retorna la región encontrada o nulo en caso de que no hayan regiones que se ajusten a lo solicitado.
region_header_t *find_region_best_fit(size_t size);


/// @brief Busca una nueva region libre mediante una determinada técnica de búsqueda.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Retorna la región encontrada o nulo en caso de que no hayan regiones que se ajusten a lo solicitado.
static region_header_t *find_free_region(size_t size);

/// @brief Agrega un nuevo bloque a la lista de bloques.
/// @param new_block Bloque a agregar a la lista.
void append_block(block_header_t *new_block);

/// @brief Calcula el tamaño del bloque necesario para albergar la región pedida
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Tamaño del bloque si la region puede ser contenida dentro de las
/// categorias. Devuelve -1 en caso de que no pueda ser contenida
int get_block_size(size_t size);

/// @brief Pide un nuevo bloque de memoria al S.O. y lo guarda en la lista.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero al comienzo de bloque, incluyendo su header. Retorna NULL en
/// caso de no poder albergar el tamaño pedido. Retorna NULL y setea errno =
/// ENOMEM en caso del que pedido de memoria falle
block_header_t *new_block(size_t size);

/// @brief Obtiene una nueva región pidiendo memoria al S.O.
/// @param size Tamaño de la región.
/// @return Puntero a la región, incluyendo su header. Retorna NULL en caso de
/// no poder pedir el tamaño pedido. Retorna NULL y setea errno = ENOMEM en caso
/// del que pedido de memoria falle.
region_header_t *new_region(size_t size);

/// @brief Divide una REGION de memoria en dos, suponiendo que ambas REGIONES
/// puedan ser potencialmente usadas. Si la REGION es divivida, se actualizara
/// el puntero de next para que apunte a la siguiente region.
/// @param region REGION a ser potencialmente dividida
/// @param size Tamaño de la REGION que debe tener en caso de ser dividida.
void split_region(region_header_t *region, size_t size);

/// @brief Realiza un split en una determinada region.
/// @param region Region a la cual se le aplicará el split.
/// @param size Tamaño que debe tener la región que luego se le retornará al usuario (Region 1).
void try_split_region(region_header_t *region, size_t size);

/// @brief Junta dos REGIONES, manteniendo la mas "izquierda" como la sobreviviente.
/// @param region REGION sobreviviente, se junta con la region next (es decir, la region a su "derecha").
/// @return Devuelve la REGION sobreviviente con los punteros modificados
region_header_t *coalesce_region_with_its_next(region_header_t *region);

/// @brief Intenta juntar las regiones adyacentes a la REGION pasada. Se
/// juntaran en caso de que alguna de las dos (o ambas) esten libres. Se intenta
/// juntar las REGIONES de "derecha" a "izquierda".
/// @param region Region la cual se chequeara si puede ser unida a sus regiones
/// adyacentes.
/// @return La REGION sobreviviente luego de intentar juntarlas. Puede ser la
/// region pasada originalmente, o la region adyacente "izquierda".
region_header_t *try_coalesce_regions(region_header_t *region);

/// @brief Devuelve un BLOQUE que se encuentra totalmente liberado al S.O.
/// Actualiza la lista de bloques
/// @param block_header Puntero al BLOQUE a liberar
void return_block_to_OS(block_header_t *block_header);

/// @brief Responde si el BLOQUE en el que esta contenido una REGION esta completamente libre
/// @param region REGION a preguntar
/// @return True en caso de que todas las regiones se encuentren libres y False en caso contrario
bool are_all_regions_free(region_header_t *region);

/// @brief Determina si la dirección de un puntero está contenida en alguno de los bloques.
/// @param ptr Puntero a verificar.
/// @return Retorna True en caso de que el puntero se encuentre contenido en alguno
/// de los bloques o False en caso contrario.
bool is_ptr_into_blocks(byte *ptr);

/// @brief Responde si un puntero es valido para ser tratado como región
/// @param ptr Puntero a verificar.
/// @return Retorna True en caso de que el puntero sea válido o False en caso contrario.
bool is_valid_ptr(byte *ptr);

/// @brief Setea los N bytes de PTR al valor C
/// @param ptr Puntero al inicio de la memoria
/// @param c Valor al que sera seteada la memoria
/// @param n Cantidad de bytes a setear
void set_mem(void *ptr, char c, size_t n);

/// @brief Mueve los primeros N bytes de src a dest
/// @param _dest Destino donde serán movidos los bytes
/// @param _src Fuente a partir de la cual se extran los bytes.
/// @param n Cantidad de bytes a mover.
void move_data(void *_dest, void *_src, int n);

/// @brief Realiza un split de la región y a la nueva region libre le aplica un
/// coalesce con la siguiente
/// @param region Región a la cual se le realiza el split
/// @param size Tamaño con el cual quedará la región luego de las operaciones
void split_and_coalesce_next_regions(region_header_t *region, size_t size);

/// @brief Maneja el realloc cuando se requiere menos memoria.
/// @param region Region brindada por el usuario.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *handle_realloc_that_requires_less_memory(region_header_t *region,
                                               size_t size);

/// @brief Responde si la region no es NULL y si esta libre
/// @param region
/// @return
bool is_free(region_header_t *region);

/// @brief Suma el tamaño de las potenciales tres regiones, teniendo en cuenta el tamaño del header
/// @param region_one No puede ser NULL
/// @param region_two Puede ser NULL
/// @param region_three Puede ser NULL
/// @return Suma del tamaño de las regiones mas el tamaño de los headers.
size_t sum_of_regions(region_header_t *region_one,
                      region_header_t *region_two,
                      region_header_t *region_three);


/// @brief Determina si las regiones están libres para aplicarles coalesce
/// @param region Region brindada por el ususario.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @param region_1 Región a verificar.
/// @param region_2 Región a verificar (opcional).
/// @return
bool are_free_to_coalesce(region_header_t *region,
                          size_t size,
                          region_header_t *region_1,
                          region_header_t *region_2);


/// @brief Realloca la memoria con la siguiente región.
/// @param region Región a ser reallocateada.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *reallocate_with_next_region(region_header_t *region, size_t size);

/// @brief Realloca la memoria con la anterior región.
/// @param region Región a ser reallocateada.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *reallocate_with_prev_region(region_header_t *region, size_t size);

/// @brief Realloca la memoria con la anterior y siguiente región.
/// @param region Región a ser reallocateada.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *reallocate_with_next_and_prev_region(region_header_t *region, size_t size);

/// @brief Realloca la memoria en una nueva región reservada con malloc. La
/// información del usuario es movida al nuevo segmento de memoria.
/// @param region Región a ser reallocateada.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *reallocate_on_new_region(region_header_t *region, size_t size);

/// @brief Maneja el realloc para el caso en que el usuario solició más memoria
/// que la que poseía su región.
/// @param region Región a ser reallocateada.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *handle_realloc_that_requires_more_memory(region_header_t *region,
                                               size_t size);

/// @brief Maneja el realloc.
/// @param region Región a ser reallocateada.
/// @param size Tamaño de memoria solicitado por el usuario.
/// @return Puntero que será devuelto al usuario.
void *handle_realloc(region_header_t *region, size_t size);

//===========================================================//
//======================== FUNCTIONS ========================//
//===========================================================//


//======================== SECONDARY ========================//


//======================== MALLOC ========================//

region_header_t *
loop_for_first_fit_region(region_header_t *start_region, size_t size)
{
	for (region_header_t *curr_region = start_region; curr_region;
	     curr_region = curr_region->next) {
		if ((curr_region->size >= size) && (curr_region->free))
			return curr_region;
	}
	return NULL;
}

static region_header_t *
find_region_first_fit(size_t size)
{
	region_header_t *found_region;

	for (block_header_t *curr_block_header = block_header_list;
	     curr_block_header;
	     curr_block_header = curr_block_header->next) {
		found_region = loop_for_first_fit_region(
		        BLOCK2REGION(curr_block_header), size);
		if (found_region)
			return found_region;
	}

	return NULL;
}

region_header_t *
loop_for_best_fit_region(region_header_t *start_region,
                         size_t size,
                         region_header_t *curr_best_region)
{
	for (region_header_t *curr_region = start_region; curr_region;
	     curr_region = curr_region->next) {
		if ((!curr_region->free) || (curr_region->size < size))
			continue;

		if ((!curr_best_region) ||
		    (curr_best_region->size > curr_region->size))
			curr_best_region = curr_region;
	}

	return curr_best_region;
}

region_header_t *
find_region_best_fit(size_t size)
{
	region_header_t *curr_best_region = NULL;

	for (block_header_t *curr_block = block_header_list; curr_block;
	     curr_block = curr_block->next) {
		curr_best_region = loop_for_best_fit_region(
		        BLOCK2REGION(curr_block), size, curr_best_region);
	}

	return curr_best_region;
}

static region_header_t *
find_free_region(size_t size)
{
#ifdef FIRST_FIT
	return find_region_first_fit(size);
#endif

#ifdef BEST_FIT
	return find_region_best_fit(size);
#endif

#ifndef FIRST_FIT
#ifndef BEST_FIT
	return find_region_first_fit(size);
#endif
#endif
}

void
append_block(block_header_t *new_block)
{
	new_block->prev = block_header_tail;
	new_block->next = NULL;

	if (!block_header_tail)
		block_header_list = new_block;
	else
		block_header_tail->next = new_block;

	block_header_tail = new_block;
}

int
get_block_size(size_t size)
{
	if (size + sizeof(region_header_t) <= BLOCK_SM)
		return BLOCK_SM;
	else if (size + sizeof(region_header_t) <= BLOCK_MD)
		return BLOCK_MD;
	else if (size + sizeof(region_header_t) <= BLOCK_LG)
		return BLOCK_LG;

	return EXIT_ERROR;
}

block_header_t *
new_block(size_t size)
{
	block_header_t *new_block_header;

	int new_block_size = get_block_size(size);
	if (new_block_size == EXIT_ERROR)
		return NULL;

	if (total_mmy + new_block_size >= HARD_LIMIT) {
		errno = ENOMEM;
		return NULL;
	}

	new_block_header = mmap(NULL,
	                        new_block_size + sizeof(block_header_t),
	                        PROT_READ | PROT_WRITE,
	                        MAP_ANON | MAP_PRIVATE,
	                        -1,
	                        0);
	if (new_block_header == MAP_FAILED) {
		errno = ENOMEM;
		return NULL;
	}

	total_mmy += new_block_size;

	INCREASE_STATS(mapped_amnt, new_block_size);
	INCREASE_STATS(total_blocks, 1);
	INCREASE_STATS(curr_blocks, 1);

	new_block_header->size = new_block_size;
	append_block(new_block_header);

	return new_block_header;
}

region_header_t *
new_region(size_t size)
{
	block_header_t *new_block_header;
	region_header_t *new_region_header;

	new_block_header = new_block(size);
	if (!new_block_header)
		return NULL;

	new_region_header = BLOCK2REGION(new_block_header);
	new_region_header->size =
	        new_block_header->size - sizeof(region_header_t);
	new_region_header->next = NULL;
	new_region_header->prev = NULL;
	new_region_header->free = true;
	new_region_header->check_num = MAGIC_32BIT;

	INCREASE_STATS(curr_regions, 1);

	return new_region_header;
}

void
split_region(region_header_t *region, size_t size)
{
	region_header_t *new_next_region;

	new_next_region = SPLITREGION(region, size);
	new_next_region->next = region->next;
	new_next_region->prev = region;
	new_next_region->size = region->size - size - sizeof(region_header_t);
	new_next_region->free = true;
	new_next_region->check_num = MAGIC_32BIT;

	region->next = new_next_region;
	region->size = size;

	INCREASE_STATS(splitted_amnt, 1);
	INCREASE_STATS(curr_regions, 1);
}

void
try_split_region(region_header_t *region, size_t size)
{
	size_t required_size_to_split;

	required_size_to_split = size + sizeof(region_header_t) + MIN_REGION_LEN;
	if (region->size < required_size_to_split) {
		INCREASE_STATS(given_amnt, region->size);
		return;
	}


	split_region(region, size);
	INCREASE_STATS(given_amnt, region->size);
}


//======================== FREE ========================//

region_header_t *
coalesce_region_with_its_next(region_header_t *region)
{
	INCREASE_STATS(coalesced_amnt, 1);
	DECREASE_STATS(curr_regions, 1);
	region_header_t *next_region = region->next;

	region->size += next_region->size + sizeof(region_header_t);
	region->next = next_region->next;
	if (next_region->next)
		next_region->next->prev = region;

	return region;
}

region_header_t *
try_coalesce_regions(region_header_t *region)
{
	if (region->next && region->next->free)
		region = coalesce_region_with_its_next(region);

	if (region->prev && region->prev->free)
		region = coalesce_region_with_its_next(region->prev);

	return region;
}

void
return_block_to_OS(block_header_t *block_header)
{
	if (block_header->prev)
		block_header->prev->next = block_header->next;
	else
		block_header_list = block_header->next;


	if (block_header->next)
		block_header->next->prev = block_header->prev;
	else
		block_header_tail = block_header->prev;

	INCREASE_STATS(returned_blocks, 1);
	DECREASE_STATS(curr_blocks, 1);
	DECREASE_STATS(mapped_amnt, block_header->size);
	DECREASE_STATS(curr_regions, 1);

	total_mmy -= block_header->size;

	munmap(block_header, block_header->size + sizeof(block_header_t));
}

bool
are_all_regions_free(region_header_t *region)
{
	return (!region->prev && !region->next && region->free);
}

bool
is_ptr_into_blocks(byte *ptr)
{
	block_header_t *curr_block_header = block_header_list;
	byte *first_dir;
	byte *last_dir;

	if (!curr_block_header)
		return false;

	while (curr_block_header) {
		first_dir = (byte *) REGION2PTR(BLOCK2REGION(curr_block_header));
		last_dir = first_dir + curr_block_header->size -
		           sizeof(region_header_t);

		if ((first_dir <= ptr) && (ptr <= last_dir))
			return true;

		curr_block_header = curr_block_header->next;
	}

	return false;
}

bool
is_valid_ptr(byte *ptr)
{
	region_header_t *region;

	// ptr check
	if (!is_ptr_into_blocks(ptr)) {
		perrorfmt("Invalid pointer\nAborted\n");
		return false;
	}

	// region check
	region = PTR2REGION(ptr);
	if (region->free) {
		perrorfmt("Double free detected\nAborted\n");
		return false;
	}
	if (region->check_num != MAGIC_32BIT) {
		perrorfmt("Invalid pointer\nAborted\n");
		return false;
	}
	return true;
}

/// @brief Setea los N bytes de PTR al valor C
/// @param ptr Puntero al inicio de la memoria
/// @param c Valor al que sera seteada la memoria
/// @param n Cantidad de bytes a setear
void
set_mem(void *ptr, char c, size_t n)
{
	byte *bptr = (byte *) ptr;

	for (size_t i = 0; i < n; i++)
		bptr[i] = c;
}

//======================== REALLOC ========================//

void
move_data(void *_dest, void *_src, int n)
{
	byte *dest = (byte *) _dest;
	byte *src = (byte *) _src;

	for (int i = 0; i < n; i++) {
		dest[i] = src[i];
	}
}

void
split_and_coalesce_next_regions(region_header_t *region, size_t size)
{
	INCREASE_STATS(coalesced_amnt, 1);
	INCREASE_STATS(splitted_amnt, 1);
	region_header_t *new_next_region;
	region_header_t *last_next_region = region->next;

	new_next_region = SPLITREGION(region, size);
	new_next_region->next = last_next_region->next;
	new_next_region->prev = last_next_region->prev;
	new_next_region->size = last_next_region->size + (region->size - size);
	new_next_region->free = true;
	new_next_region->check_num = MAGIC_32BIT;

	region->next = new_next_region;
	region->size = size;
}

void *
handle_realloc_that_requires_less_memory(region_header_t *region, size_t size)
{
	INCREASE_STATS(realloc_optimized, 1);

	if ((region->next) && (region->next->free)) {
		split_and_coalesce_next_regions(region, size);
	} else
		try_split_region(region, size);

	return REGION2PTR(region);
}


bool
is_free(region_header_t *region)
{
	return (region && region->free);
}


size_t
sum_of_regions(region_header_t *region_one,
               region_header_t *region_two,
               region_header_t *region_three)
{
	size_t sum = region_one->size;
	if (region_two)
		sum += region_two->size + sizeof(region_header_t);
	if (region_three)
		sum += region_three->size + sizeof(region_header_t);
	return sum;
}

bool
are_free_to_coalesce(region_header_t *region,
                     size_t size,
                     region_header_t *region_1,
                     region_header_t *region_2)
{
	if (!region_2) {
		return (is_free(region_1) &&
		        sum_of_regions(region, region_1, NULL) >= size);
	}

	return (is_free(region_1) && (is_free(region_2)) &&
	        sum_of_regions(region, region_1, region_2) >= size);
}

void *
reallocate_with_next_region(region_header_t *region, size_t size)
{
	INCREASE_STATS(realloc_optimized, 1);
	region = coalesce_region_with_its_next(region);

	region->free = false;
	try_split_region(region, size);
	return REGION2PTR(region);
}

void *
reallocate_with_prev_region(region_header_t *region, size_t size)
{
	INCREASE_STATS(realloc_optimized, 1);
	region_header_t *last_region = region;

	region = coalesce_region_with_its_next(region->prev);
	move_data(REGION2PTR(region), REGION2PTR(last_region), last_region->size);

	region->free = false;
	try_split_region(region, size);
	return REGION2PTR(region);
}

void *
reallocate_with_next_and_prev_region(region_header_t *region, size_t size)
{
	INCREASE_STATS(realloc_optimized, 1);
	region_header_t *last_region = region;

	region = coalesce_region_with_its_next(region->prev);
	region = coalesce_region_with_its_next(region);
	move_data(REGION2PTR(region), REGION2PTR(last_region), last_region->size);

	region->free = false;
	try_split_region(region, size);
	return REGION2PTR(region);
}

void *
reallocate_on_new_region(region_header_t *region, size_t size)
{
	INCREASE_STATS(realloc_no_optimized, 1);
	void *new_ptr = malloc(size);
	if (!new_ptr)
		return NULL;

	move_data(new_ptr, REGION2PTR(region), region->size);
	free(REGION2PTR(region));
	return new_ptr;
}

void *
handle_realloc_that_requires_more_memory(region_header_t *region, size_t size)
{
	if (are_free_to_coalesce(region, size, region->next, NULL)) {
		return reallocate_with_next_region(region, size);
	} else if (are_free_to_coalesce(region, size, region->prev, NULL)) {
		return reallocate_with_prev_region(region, size);
	} else if (are_free_to_coalesce(region, size, region->prev, region->next)) {
		return reallocate_with_next_and_prev_region(region, size);
	}

	return reallocate_on_new_region(region, size);
}

void *
handle_realloc(region_header_t *region, size_t size)
{
	if (region->size == size)
		return REGION2PTR(region);

	if (region->size > size)
		return handle_realloc_that_requires_less_memory(region, size);

	return handle_realloc_that_requires_more_memory(region, size);
}


//======================== PRINCIPAL ========================//

void *
malloc(size_t size)
{
	INCREASE_STATS(malloc_calls, 1);
	INCREASE_STATS(requested_amnt, size);
	region_header_t *region;

	if (size == 0)
		return NULL;

	if (ALIGN4(size) + sizeof(region_header_t) > BLOCK_LG) {
		errno = ENOMEM;
		return NULL;
	}

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);
	if (size < MIN_REGION_LEN)
		size = MIN_REGION_LEN;

	region = find_free_region(size);
	if (!region) {
		region = new_region(size);
		if (!region)
			return NULL;
	}

	region->free = false;
	try_split_region(region, size);

	return REGION2PTR(region);
}

void *
calloc(size_t nmemb, size_t size)
{
	INCREASE_STATS(calloc_calls, 1);

	// handle overflow
	if ((size != 0) && (nmemb > __SIZE_MAX__ / size)) {
		errno = ENOMEM;
		return NULL;
	}

	void *ptr = malloc(size * nmemb);
	if (!ptr)
		return NULL;

	set_mem(ptr, 0, PTR2REGION(ptr)->size);
	return ptr;
}

void *
realloc(void *ptr, size_t size)
{
	INCREASE_STATS(realloc_calls, 1);

	if (!ptr)
		return malloc(size);

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	if (!is_valid_ptr((byte *) ptr)) {
		exit(INVALID_POINTER);
	}

	region_header_t *region = PTR2REGION(ptr);

	INCREASE_STATS(requested_amnt, size - region->size);

	size = ALIGN4(size);
	if (size + sizeof(region_header_t) > BLOCK_LG) {
		errno = ENOMEM;
		return NULL;
	}

	if (size < MIN_REGION_LEN) {
		size = MIN_REGION_LEN;
	}

	return handle_realloc(PTR2REGION(ptr), size);
}

void
free(void *ptr)
{
	if (!ptr)
		return;

	region_header_t *region_to_free;

	if (!is_valid_ptr((byte *) ptr)) {
		exit(INVALID_POINTER);
	}

	region_to_free = PTR2REGION(ptr);
	region_to_free->free = true;

	INCREASE_STATS(free_calls, 1);
	INCREASE_STATS(freed_amnt, region_to_free->size);

	region_to_free = try_coalesce_regions(region_to_free);

	if (are_all_regions_free(region_to_free))
		return_block_to_OS(REGION2BLOCK(region_to_free));
}
