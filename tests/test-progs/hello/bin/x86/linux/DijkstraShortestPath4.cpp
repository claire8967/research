#include <bits/stdc++.h>
#include <iostream>


using namespace std;
void showpath(int path[],int i)
{
    if (path[i]==-1)
    return;
    showpath(path,path[i]);
    cout<<char('A'+i)<<"\t";

}
int mindist(int dist[],char traversed[],int n)
{
    int min=9999;
    int min_pos;
    for(int i=0;i<n;i++){
        if (traversed[i] == '\0' && dist[i] <= min)
        {
            min = dist[i];
            min_pos = i;
        }
    }
    return min_pos;
}
void printDistance(int matrix[][100], int n)
{
    char vertice1 = 'A';
    char vertice2 = 'A';
    for (int i = -1; i < n; i++)
    {
        for (int j = -1; j < n; j++)
        {
            if (i == -1 && j == -1)
            {
                cout << "Dist "<< "\t";
            }
            else if (i == -1 && j != -1)
            {
                cout << vertice1++ << "\t";
            }
            else if (j == -1 && i != -1)
            {
                cout << vertice2++ << "\t";
            }
            else
                cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }
    cout << endl;
}
void printSolution(int dist[],int n,int path[])
{
    cout<<"Vertex \t\t Distance from Source\t\tPath"<<endl;
    for (int i = 0; i < n; i++){
        cout<<endl<<'A'<<"->"<<char('A'+i)<<"\t\t"<<dist[i]<<"\t\t\t\t"<<'A'<<"\t";
        showpath(path,i);

    }
}

void dijktras(int weight[][1000],int n,int source)
{
    int path[n];
    int dist[n];
    char traversed[n];
    for (int i=0;i<n;i++)
    {
        if (i!=source)
        {
            dist[i]=9999;
        }
        else
        {
            dist[i]=0;
            path[i]=-1;
        }
        traversed[i]='\0';
        
    }
    int min=9999;
    int min_pos;
    for (int i=0;i<n-1;i++)
    {
        int min_pos=mindist(dist,traversed,n);
        traversed[min_pos] = char('A' + i);

        for (int j=0;j<n;j++)
        {
            if(traversed[j]=='\0'&& weight[min_pos][j] && dist[min_pos]!=9999 && dist[min_pos]+weight[min_pos][j]<dist[j])
            {
                dist[j]=dist[min_pos]+weight[min_pos][j];
                path[j]=min_pos;
            }
        }
    }
    //printSolution(dist,n,path);
}
int main()
{
    int n = 1000;

    // Allocate memory for weight arrays dynamically on the heap
    int (*weight)[1000] = new int[n][1000];
    int (*weight1)[1000] = new int[n][1000];
    int (*weight2)[1000] = new int[n][1000];
    int (*weight3)[1000] = new int[n][1000];
    // int (*weight4)[1000] = new int[n][1000];
    // int (*weight5)[1000] = new int[n][1000];
    // int (*weight6)[1000] = new int[n][1000];
    // int (*weight7)[1000] = new int[n][1000];
    // int (*weight8)[1000] = new int[n][1000];
    // int (*weight9)[1000] = new int[n][1000];
    // int (*weight10)[1000] = new int[n][1000];
    // int (*weight11)[1000] = new int[n][1000];
    // int (*weight12)[1000] = new int[n][1000];

    // Populate the weight arrays
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            weight[i][j] = rand()%10+1;
            weight1[i][j] = rand()%10+1;
            weight2[i][j] = rand()%10+1;
            weight3[i][j] = rand()%10+1;
            // weight4[i][j] = rand()%10+1;
            // weight5[i][j] = rand()%10+1;
            // weight6[i][j] = rand()%10+1;
            // weight7[i][j] = rand()%10+1;
            // weight8[i][j] = rand()%10+1;
            // weight9[i][j] = rand()%10+1;
            // weight10[i][j] = rand()%10+1;
            // weight11[i][j] = rand()%10+1;
            // weight12[i][j] = rand()%10+1;

        }
    }

    // Call dijkstras for each weight array
    dijktras(weight, n, 0);
    dijktras(weight1, n, 0);
    dijktras(weight2, n, 0);
    dijktras(weight3, n, 0);
    // dijktras(weight4, n, 0);
    // dijktras(weight5, n, 0);
    // dijktras(weight6, n, 0);
    // dijktras(weight7, n, 0);
    // dijktras(weight8, n, 0);
    // dijktras(weight9, n, 0);
    // dijktras(weight10, n, 0);
    // dijktras(weight11, n, 0);
    // dijktras(weight12, n, 0);

    // Free dynamically allocated memory
    delete[] weight;
    delete[] weight1;
    delete[] weight2;
    delete[] weight3;
    // delete[] weight4;
    // delete[] weight5;
    // delete[] weight6;
    // delete[] weight7;
    // delete[] weight8;
    // delete[] weight9;
    // delete[] weight10;
    // delete[] weight11;
    // delete[] weight12;
}
