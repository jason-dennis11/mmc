#include <stdlib.h>
#include "simpmesh.h"

void vec_diff(float3 *a,float3 *b,float3 *res){
	res->x=b->x-a->x;
	res->y=b->y-a->y;
	res->z=b->z-a->z;
}
void vec_mult_add(float3 *a,float3 *b,float sa,float sb,float3 *res){
	res->x=sb*b->x+sa*a->x;
	res->y=sb*b->y+sa*a->y;
	res->z=sb*b->z+sa*a->z;
}
void vec_cross(float3 *a,float3 *b,float3 *res){
	res->x=a->y*b->z-a->z*b->y;
	res->y=a->z*b->x-a->x*b->z;
	res->z=a->x*b->y-a->y*b->x;
}
float vec_dot(float3 *a,float3 *b){
	return a->x*b->x+a->y*b->y+a->z*b->z;
}
float pinner(float3 *Pd,float3 *Pm,float3 *Ad,float3 *Am){
	return vec_dot(Pd,Am)+vec_dot(Pm,Ad);
}

void mesh_init(tetmesh *mesh){
	mesh->nn=0;
	mesh->ne=0;
	mesh->prop=0;
	mesh->node=NULL;
	mesh->elem=NULL;
	mesh->facenb=NULL;
	mesh->type=NULL;
	mesh->med=NULL;
	mesh->weight=NULL;
}
void mesh_error(char *msg){
	fprintf(stderr,"%s\n",msg);
	exit(1);
}
void mesh_loadnode(tetmesh *mesh,char *fnode){
	FILE *fp;
	int tmp,len,i;
	if((fp=fopen(fnode,"rt"))==NULL){
		mesh_error("can not open node file");
	}
	len=fscanf(fp,"%d %d",&tmp,&(mesh->nn));
	if(len!=2 || mesh->nn<=0){
		mesh_error("mesh file has wrong format");
	}
	mesh->node=(float3 *)malloc(sizeof(float3)*mesh->nn);
	mesh->weight=(float *)calloc(sizeof(float)*mesh->nn,1);

	for(i=0;i<mesh->nn;i++){
		if(fscanf(fp,"%d %f %f %f",&tmp,&(mesh->node[i].x),&(mesh->node[i].y),&(mesh->node[i].z))!=4)
			mesh_error("mesh file has wrong format");
	}
	fclose(fp);
}

void mesh_loadmedia(tetmesh *mesh,char *fmed){
	FILE *fp;
	int tmp,len,i;
	if((fp=fopen(fmed,"rt"))==NULL){
		mesh_error("can not open media property file");
	}
	len=fscanf(fp,"%d %d",&tmp,&(mesh->prop));
	if(len!=2 || mesh->prop<=0){
		mesh_error("property file has wrong format");
	}
	mesh->med=(medium *)malloc(sizeof(medium)*mesh->prop);
	for(i=0;i<mesh->prop;i++){
		if(fscanf(fp,"%d %f %f %f %f",&tmp,&(mesh->med[i].mua),&(mesh->med[i].musp),
		                                   &(mesh->med[i].g),&(mesh->med[i].n))!=5)
			mesh_error("property file has wrong format");
	}
	fclose(fp);
}
void mesh_loadelem(tetmesh *mesh,char *felem){
	FILE *fp;
	int tmp,len,i;
	int4 *pe;

	if((fp=fopen(felem,"rt"))==NULL){
		mesh_error("can not open element file");
	}
	len=fscanf(fp,"%d %d",&tmp,&(mesh->ne));
	if(len!=2 || mesh->ne<=0){
		mesh_error("mesh file has wrong format");
	}
	mesh->elem=(int4 *)malloc(sizeof(int4)*mesh->ne);
	mesh->type=(int  *)malloc(sizeof(int )*mesh->ne);

	for(i=0;i<mesh->ne;i++){
		pe=mesh->elem+i;
		if(fscanf(fp,"%d %d %d %d %d %d",&tmp,&(pe->x),&(pe->y),&(pe->z),&(pe->w),mesh->type+i)!=6)
			mesh_error("mesh file has wrong format");
	}
	fclose(fp);
}
void mesh_loadfaceneighbor(tetmesh *mesh,char *ffacenb){
	FILE *fp;
	int tmp,len,i;
	int4 *pe;

	if((fp=fopen(ffacenb,"rt"))==NULL){
		mesh_error("can not open element file");
	}
	len=fscanf(fp,"%d %d",&tmp,&(mesh->ne));
	if(len!=2 || mesh->ne<=0){
		mesh_error("mesh file has wrong format");
	}
	mesh->facenb=(int4 *)malloc(sizeof(int4)*mesh->ne);
	for(i=0;i<mesh->ne;i++){
		pe=mesh->facenb+i;
		if(fscanf(fp,"%d %d %d %d",&(pe->x),&(pe->y),&(pe->z),&(pe->w))!=4)
			mesh_error("mesh file has wrong format");
	}
	fclose(fp);
}
void mesh_clear(tetmesh *mesh){
	mesh->nn=0;
	mesh->ne=0;
	if(mesh->node){
		free(mesh->node);
		mesh->node=NULL;
	}
	if(mesh->elem){
		free(mesh->elem);
		mesh->elem=NULL;
	}
	if(mesh->facenb){
		free(mesh->facenb);
		mesh->facenb=NULL;
	}
	if(mesh->type){
		free(mesh->type);
		mesh->type=NULL;
	}
	if(mesh->med){
		free(mesh->med);
		mesh->med=NULL;
	}
	if(mesh->weight){
		free(mesh->weight);
		mesh->weight=NULL;
	}
}

void plucker_init(tetplucker *plucker,tetmesh *pmesh){
	plucker->d=NULL;
	plucker->m=NULL;
	plucker->mesh=pmesh;
	plucker_build(plucker);
}
void plucker_clear(tetplucker *plucker){
	if(plucker->d) {
		free(plucker->d);
		plucker->d=NULL;
	}
	if(plucker->m) {
		free(plucker->m);
		plucker->m=NULL;
	}	
	plucker->mesh=NULL;
}
void plucker_build(tetplucker *plucker){
	int nn,ne,i,j;
	int pairs[6][2]={{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
	float3 *nodes;
	int *elems,ebase;
	int e1,e0;
	
	if(plucker->d || plucker->m || plucker->mesh==NULL) return;
	nn=plucker->mesh->nn;
	ne=plucker->mesh->ne;
	nodes=plucker->mesh->node;
	elems=(int *)(plucker->mesh->elem); // convert int4* to int*
	plucker->d=(float3*)malloc(sizeof(float3)*ne*6); // 6 edges/elem
	plucker->m=(float3*)malloc(sizeof(float3)*ne*6); // 6 edges/elem
	for(i=0;i<ne;i++){
		ebase=i<<2;
		for(j=0;j<6;j++){
			e1=elems[ebase+pairs[j][1]]-1;
			e0=elems[ebase+pairs[j][0]]-1;
			vec_diff(&nodes[e0],&nodes[e1],plucker->d+i*6+j);
			vec_cross(&nodes[e0],&nodes[e1],plucker->m+i*6+j);
		}
	}
}

float dist2(float3 *p0,float3 *p1){
    return (p1->x-p0->x)*(p1->x-p0->x)+(p1->y-p0->y)*(p1->y-p0->y)+(p1->z-p0->z)*(p1->z-p0->z);
}

float dist(float3 *p0,float3 *p1){
    return sqrt(dist2(p0,p1));
}

float rand01(){
    return rand()*R_RAND_MAX;
}

void sincosf(float theta,float *stheta,float *ctheta){
    *stheta=sin(theta);
    *ctheta=cos(theta);
}

void mc_next_scatter(float g, float musp, float3 *pnext){
    float nextlen=rand01();
    float sphi,cphi,tmp0,theta,stheta,ctheta,tmp1;
    float3 p;

    nextlen=((nextlen==0.f)?LOG_MT_MAX:(-log(nextlen)));
    
    //random arimuthal angle
    tmp0=TWO_PI*rand01(); //next arimuth angle
    sincosf(tmp0,&sphi,&cphi);

    //Henyey-Greenstein Phase Function, "Handbook of Optical Biomedical Diagnostics",2002,Chap3,p234
    //see Boas2002

    if(g>EPS){  //if g is too small, the distribution of theta is bad
	tmp0=(1.f-g*g)/(1.f-g+2.f*g*rand01());
	tmp0*=tmp0;
	tmp0=(1.f+g*g-tmp0)/(2.f*g);

    	// when ran=1, CUDA will give me 1.000002 for tmp0 which produces nan later
    	if(tmp0>1.f) tmp0=1.f;

	theta=acosf(tmp0);
	stheta=sinf(theta);
	ctheta=tmp0;
    }else{  //Wang1995 has acos(2*ran-1), rather than 2*pi*ran, need to check
	theta=M_PI*rand01();
    	sincosf(theta,&stheta,&ctheta);
    }

    if( pnext->z>-1.f+EPS && pnext->z<1.f-EPS ) {
	tmp0=1.f-pnext->z*pnext->z;   //reuse tmp to minimize registers
	tmp1=1.f/sqrtf(tmp0);
	tmp1=stheta*tmp1;

	p.x=tmp1*(pnext->x*pnext->z*cphi - pnext->y*sphi) + pnext->x*ctheta;
	p.y=tmp1*(pnext->y*pnext->z*cphi + pnext->x*sphi) + pnext->y*ctheta;
	p.z=-tmp1*tmp0*cphi			       + pnext->z*ctheta;
    }else{
	p.x=stheta*cphi;
	p.y=stheta*sphi;
	p.z=(pnext->z>0.f)?ctheta:-ctheta;
    }
    pnext->x+=nextlen*p.x;
    pnext->y+=nextlen*p.y;
    pnext->z+=nextlen*p.z;
}