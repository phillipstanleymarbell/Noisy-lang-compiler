#include<stdio.h>
#include<math.h>

int
printInt32(int x)
{
        return printf("%d\n",x);
}

int
printFloat64(double x)
{
        return printf("%lf\n",x);
}

int
readInt32()
{
        int x;
        scanf("%d",&x);
        return x;
}

short int
readInt16FromCSV()
{
        int x;
        short int y;
        scanf("%d,%hd",&x,&y);
        return y;
}

double
readFloat64FromCSV()
{
        double x,y;
        scanf("%lf,%lf",&x,&y);
        return y;
}

double
readStartFloat64FromCSV()
{
        double x;
        scanf("%lf",&x);
        return x;
}

double
readMiddleFloat64FromCSV()
{
        double x;
        scanf(",%lf",&x);
        return x;
}

int
ekfWrite(double ts, double theta, double dtheta, double thetaCov, double dthetaCov)
{
        return printf("%.6f,%.15f,%.15f,%.15f,%.15f\n",ts,theta,dtheta,thetaCov,dthetaCov);
}

float
readFloat32FromCSV()
{
        float x;
        scanf("%f",&x);
        return x;
}

float
noisyAbs(float x)
{
        return fabsf(x);
}