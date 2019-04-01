#include <omp.h>
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <iostream>
using namespace std;

//#define DEBUG

/*!
* quickSort - реализация алгоритма быстрой сортировки
* @param num номер вызова функции (для отладки)
* @param a сортируемый массив
* @param n индекс последнего элемента массива (не размер массива!)
*/
void quickSort(int num, int* a, const long n) {
	long i = 0, j = n;
	int	pivot = a[n / 2]; /*Опорный элемент*/
	
#ifdef DEBUG /*Если программа сломается - печатаем номер функции на которой она "упала"*/
	#pragma omp critical /*Входим сюда, только если эта часть не обрабатывается другим потоком*/
		{
			printf("enter: %d\n", num);
		}
#endif

	do {
		while (a[i] < pivot) i++;
		while (a[j] > pivot) j--;

		if (i <= j) {
      		swap(a[i], a[j]);
			i++; j--;
		}
	} while (i <= j);

#ifdef DEBUG /*Отладка*/
	#pragma omp critical
		{
			printf("divide #%d by %3.2f [%d] -> (%d|%d) : %d\n", num, pivot, n/2, j, n - i, n);
		}
#endif
	/*Если у нас небольшой массив - сортировка выполняется последовательно*/
	if (n < 100) { 
		if (j > 0) quickSort(num * 2 + 1, a, j);
		if (n > i) quickSort(num * 2 + 2, a + i, n - i);
		return;
	}
	
#pragma omp task shared(a)
	{
		/*int m = omp_get_thread_num();     //Печатаем на каком потоке выполняется сортировка
		printf("\nMy flow: %d\n", m);*/
		if (j > 0) quickSort(num * 2 + 1, a, j);
	} // #pragma omp task
#pragma omp task shared(a)
	{
		/*int w = omp_get_thread_num();
		printf("\nMy flow: %d\n", w);*/
		if (n > i) quickSort(num * 2 + 2, a + i, n - i);
	} // #pragma omp task
#pragma omp taskwait /*Ждём завершения всех задач*/
}



int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("error arg\n");
		return 1;
  }

	int *a;
	long n;

	double start, end;
	int flowes = 16; /*Кол-во потоков*/
	start = omp_get_wtime();

  omp_set_num_threads(flowes);

  sscanf(argv[1], "%ld", &n);

	a = new int[n];

	srand(time(NULL)); /*Заполняем массив рандомными элементами*/
	for (int i = 0; i < n; ++i)
		a[i] = rand() % 100;
	
#ifdef DEBUG /*Печать массива при отладке*/
	for (int i = 0; i < n; ++i)
		printf("%d ", a[i]);
	printf("\n");
#endif
	
#pragma omp parallel shared(a) /*Создаём потоки до входа в функцию*/
	{
		#pragma omp single nowait /*Вызываем функцию только 1 раз*/ 
		{
			quickSort(0, a, n - 1);
		} // #pragma omp single
	} // #pragma omp parallel
	
  /* Проверка правильности сортировки */
	/*for (int i = 1; i < n; ++i) {
		if (a[i] < a[i-1])
      printf("error\n");
	}*/

	for (int i = 1; i < n; ++i)
		printf("%d ",a[i]);

  	delete []a;
	end = omp_get_wtime();
	cout << endl << "Work took " << end-start << " seconds!" << endl;
}
