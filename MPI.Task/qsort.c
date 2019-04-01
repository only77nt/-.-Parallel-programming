/* quicksort */
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>

#define N 100

double startTime, stopTime; /*Можно было воспользоваться функцией MPI_Wtime, но я решил использовать библиотечную функцию*/

void showElapsed(int id, char *m) /*Функция, показывающая прошедшее время*/
{
	printf("%d: %s %f secs\n",id,m,(clock()-startTime)/CLOCKS_PER_SEC);
}

void showVector(int *v, int n, int id) /*Функция печати массива в командной строке*/
{
	int i;
	printf("%d: ",id);
	for(i=0;i<n;i++)
		printf("%d ",v[i]);
	putchar('\n');
}

int * merge(int *v1, int n1, int *v2, int n2) /*Функция слияния массивов*/
{
	int i,j,k;
	int * result;

	result = (int *)malloc((n1+n2)*sizeof(int));

	i=0; j=0; k=0;
	while(i<n1 && j<n2)
		if(v1[i]<v2[j])
		{
			result[k] = v1[i];
			i++; k++;
		}
		else
		{
			result[k] = v2[j];
			j++; k++;
		}
	if(i==n1)
		while(j<n2)
		{
			result[k] = v2[j];
			j++; k++;
		}
	else
		while(i<n1)
		{
			result[k] = v1[i];
			i++; k++;
		}
	return result;
}

void swap(int *v, int i, int j) /*Функция, меняющая элементы местами*/
{
	int t;
	t = v[i];
	v[i] = v[j];
	v[j] = t;
}

void qsort1(int *v, int left, int right) /*Сама сортировка*/
{
	int i,last;
	if(left>=right)
		return;
	swap(v,left,(left+right)/2);
	last = left;
	for(i=left+1;i<=right;i++)
		if(v[i]<v[left])
			swap(v,++last,i);
	swap(v,left,last);
	qsort1(v,left,last-1);
	qsort1(v,last+1,right);
}

/*MAIN*/
int main(int argc, char **argv)
{
	int * data;
	int * chunk;
	int * other;
	int m,n=atoi(argv[1]); /*n - число элементов массива (можно вернуть N)*/
	int id,p; /*p - число ядер (нитей)*/
	int s;
	int i;
	int step;
	MPI_Status status;

	startTime = clock();

	/*Начало параллельной части программы*/
	MPI_Init(&argc,&argv); /*Инициализация, получение аргументов командной строки*/
	MPI_Comm_rank(MPI_COMM_WORLD,&id); /*Номера процессов*/
	MPI_Comm_size(MPI_COMM_WORLD,&p); /*Общее кол-во процессов*/

	showElapsed(id,"MPI setup complete");

	if(id==0)
	{
		int r;
		srandom(clock());

		/*Генерируем наш массив*/		
		s = n/p;
		r = n%p;
		data = (int *)malloc((n+s-r)*sizeof(int));
		for(i=0;i<n;i++)
			data[i] = random()%100;
		if(r!=0)
		{
			for(i=n;i<n+s-r;i++)
				data[i]=0;
			s=s+1;
		}
		showElapsed(id,"generated the random numbers");

		/*Разбиваем его на подмассивы и начинаем сортировку*/
		MPI_Bcast(&s,1,MPI_INT,0,MPI_COMM_WORLD); /*Рассылаем размер массива всем процессам*/
		chunk = (int *)malloc(s*sizeof(int));
		MPI_Scatter(data,s,MPI_INT,chunk,s,MPI_INT,0,MPI_COMM_WORLD); /*Разделяем массив и раскидываем его кусочки по процессам*/

		showElapsed(id,"scattered data");

		qsort1(chunk,0,s-1);

		showElapsed(id,"sorted");
	}
	else
	{
		MPI_Bcast(&s,1,MPI_INT,0,MPI_COMM_WORLD); /*Рассылаем размер массива всем процессам*/
		chunk = (int *)malloc(s*sizeof(int));
		MPI_Scatter(data,s,MPI_INT,chunk,s,MPI_INT,0,MPI_COMM_WORLD); /*Разделяем массив и раскидываем его кусочки по процессам*/

		showElapsed(id,"got data");

		qsort1(chunk,0,s-1);

		showElapsed(id,"sorted");
	}

	step = 1;
	while(step<p)
	{
		if(id%(2*step)==0)
		{
			if(id+step<p)
			{
				/*Получение информации процессом*/
				MPI_Recv(&m,1,MPI_INT,id+step,0,MPI_COMM_WORLD,&status);
				other = (int *)malloc(m*sizeof(int));
				MPI_Recv(other,m,MPI_INT,id+step,0,MPI_COMM_WORLD,&status);
				showElapsed(id,"got merge data");
				chunk = merge(chunk,s,other,m);
				showElapsed(id,"merged data");
				s = s+m;
			} 
		}
		else
		{
			/*Отправка информации другим процессам*/
			int near = id-step;
			MPI_Send(&s,1,MPI_INT,near,0,MPI_COMM_WORLD);
			MPI_Send(chunk,s,MPI_INT,near,0,MPI_COMM_WORLD);
			showElapsed(id,"sent merge data");
			break;
		}
		step = step*2;
	}

	/*По окончанию сортировки записываем полученный результат в файл*/	
	if(id==0)
	{
		FILE * fout;

		stopTime = clock();
		printf("%d; %d processors; %f secs\n", s,p,(stopTime-startTime)/CLOCKS_PER_SEC); /*В начале печатается кол-во отсортированных элементов*/

		showElapsed(id,"opening out file");
		fout = fopen("result","w");
		for(i=0;i<s;i++)
			fprintf(fout,"%d\n",chunk[i]);
		fclose(fout);
		showElapsed(id,"closed out file");
	}

	MPI_Finalize(); /*Выход из параллельной части программы*/

return 0;
}
