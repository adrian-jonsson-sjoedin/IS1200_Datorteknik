#include <stdio.h>
//*x implies x[0] and *(x+i) in turn implies x[i]
void square_reverse(double *x, double *y, const int len){
  y += len - 1; //makes it so that y will have the correct length 
  for(int i = 0; i < len; i++){
    *y = (*x) * (*x); //rewrites the value at the addres y is currently 
                      //poining at
    y--; //decrement the pointer for out 
    x++; //increment pointer for next element fetch
  }
}
int main(){
  double in[] = {11.0, 20.0, 100.0};
  double out[3];
  square_reverse(in, out, 3);
  /*for loop to check that our code is working */
  for(int i =0; i < 3; i++){
    printf("%f ", out[i]);
  }
  return 0;
}

/* 
 *What I looked at to understand how passing array to function using call by refernce works
 https://beginnersbook.com/2014/01/c-passing-array-to-function-example/ (loaded 22-01-23)
 *
#include <stdio.h>
void myfuncn( int *var1, int var2)
{
	* The pointer var1 is pointing to the first element of
	 * the array and the var2 is the size of the array. In the
	 * loop we are incrementing pointer so that it points to
	 * the next element of the array on each increment.
	 *
	 *
    for(int x=0; x<var2; x++)
    {
        printf("Value of var_arr[%d] is: %d \n", x, *var1);
        //increment pointer for next element fetch*
        var1++;
    }
}

int main()
{
     int var_arr[] = {11, 22, 33, 44, 55, 66, 77};
     myfuncn(var_arr, 7);
     return 0;
}
*/
