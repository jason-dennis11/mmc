#include "tettracing.h"

void interppos(float3 *w,float3 *p1,float3 *p2,float3 *p3,float3 *pout){
	pout->x=w->x*p1->x+w->y*p2->x+w->z*p3->x;
	pout->y=w->x*p1->y+w->y*p2->y+w->z*p3->y;
	pout->z=w->x*p1->z+w->y*p2->z+w->z*p3->z;
}
void getinterp(float w1,float w2,float w3,float3 *p1,float3 *p2,float3 *p3,float3 *pout){
        pout->x=w1*p1->x+w2*p2->x+w3*p3->x;
        pout->y=w1*p1->y+w2*p2->y+w3*p3->y;
        pout->z=w1*p1->z+w2*p2->z+w3*p3->z;
}

void trackpos(float3 *p0,float3 *p1,tetplucker *plucker,int eid /*start from 1*/, 
              float3 *pout, float dlen, int *faceid, float *weight, int *isend,Config *cfg){
	float3 pvec, pcrx;
	float3 pin;
	int *ee;
	int i;
	float w[6],Rv,ww;
	int fc[4][3]={{0,4,2},{3,5,4},{2,5,1},{1,3,0}};
	int nc[4][3]={{3,0,1},{3,1,2},{2,0,3},{1,0,2}};
	int faceorder[]={1,3,2,0};
        float bary[2][4]={{0.f,0.f,0.f,0.f},{0.f,0.f,0.f,0.f}};

	if(plucker->mesh==NULL || plucker->d==NULL||eid<=0||eid>plucker->mesh->ne) 
		return;

	*faceid=-1;
	*isend=0;
	vec_diff(p0,p1,&pvec);
	vec_cross(p0,p1,&pcrx);
	ee=(int *)(plucker->mesh->elem+eid-1);
	for(i=0;i<6;i++){
		w[i]=pinner(&pvec,&pcrx,plucker->d+(eid-1)*6+i,plucker->m+(eid-1)*6+i);
		if(cfg->debuglevel&dlTracing) fprintf(cfg->flog,"%f ",w[i]);
	}
	if(cfg->debuglevel&dlTracing) fprintf(cfg->flog,"%d \n",eid);

	pin.x=QLIMIT;
	pout->x=QLIMIT;
	for(i=0;i<4;i++){
	    if(w[fc[i][0]]||w[fc[i][1]]||w[fc[i][2]]){
		if(cfg->debuglevel&dlTracing) fprintf(cfg->flog,"testing face [%d]\n",i);

	        if(i>=2) w[fc[i][1]]=-w[fc[i][1]]; // can not go back
		if(pin.x==QLIMIT&&w[fc[i][0]]<=0 && w[fc[i][1]]<=0 && w[fc[i][2]]>=0){
			// f_enter
                        if(cfg->debuglevel&dlTracingEnter) fprintf(cfg->flog,"ray enters face %d[%d] of %d\n",i,faceorder[i],eid);

                        Rv=1.f/(-w[fc[i][0]]-w[fc[i][1]]+w[fc[i][2]]);
                        bary[0][nc[i][0]]=-w[fc[i][0]]*Rv;
                        bary[0][nc[i][1]]=-w[fc[i][1]]*Rv;
                        bary[0][nc[i][2]]=w[fc[i][2]]*Rv;

			getinterp(bary[0][nc[i][0]],bary[0][nc[i][1]],bary[0][nc[i][2]],
				plucker->mesh->node+ee[nc[i][0]]-1,
				plucker->mesh->node+ee[nc[i][1]]-1,
				plucker->mesh->node+ee[nc[i][2]]-1,&pin);

                        if(cfg->debuglevel&dlTracingEnter) fprintf(cfg->flog,"entrance point %f %f %f\n",pin.x,pin.y,pin.z);

		}else if(pout->x==QLIMIT&&w[fc[i][0]]>=0 && w[fc[i][1]]>=0 && w[fc[i][2]]<=0){
			// f_leave
                        if(cfg->debuglevel&dlTracingExit) fprintf(cfg->flog,"ray exits face %d[%d] of %d\n",i,faceorder[i],eid);

                        Rv=1.f/(w[fc[i][0]]+w[fc[i][1]]-w[fc[i][2]]);
                        bary[1][nc[i][0]]=w[fc[i][0]]*Rv;
                        bary[1][nc[i][1]]=w[fc[i][1]]*Rv;
                        bary[1][nc[i][2]]=-w[fc[i][2]]*Rv;

			getinterp(bary[1][nc[i][0]],bary[1][nc[i][1]],bary[1][nc[i][2]],
				plucker->mesh->node+ee[nc[i][0]]-1,
				plucker->mesh->node+ee[nc[i][1]]-1,
				plucker->mesh->node+ee[nc[i][2]]-1,pout);

                        if(cfg->debuglevel&dlTracingExit) fprintf(cfg->flog,"exit point %f %f %f\n",pout->x,pout->y,pout->z);

			*faceid=faceorder[i];
			*isend=(dist2(p0,pout)>dist2(p0,p1));

			if(*isend)
			    *weight*=exp(-plucker->mesh->med[plucker->mesh->type[eid-1]-1].mua*dist(p0,p1));
			else
			    *weight*=exp(-plucker->mesh->med[plucker->mesh->type[eid-1]-1].mua*dist(p0,pout));

                        if(cfg->debuglevel&dlWeight) fprintf(cfg->flog,"update weight to %f and path end %d \n",*weight,*isend);

/*
			Rv*=(*weight)*dlen;
			plucker->mesh->weight[ee[0]-1]+=*weight*0.25f;
			plucker->mesh->weight[ee[1]-1]+=*weight*0.25f;
			plucker->mesh->weight[ee[2]-1]+=*weight*0.25f;
			plucker->mesh->weight[ee[3]-1]+=*weight*0.25f;

			plucker->mesh->weight[ee[nc[i][0]]-1]+=Rv*w[fc[i][0]];
			plucker->mesh->weight[ee[nc[i][1]]-1]+=Rv*w[fc[i][1]];
			plucker->mesh->weight[ee[nc[i][2]]-1]+=-Rv*w[fc[i][2]];
*/
		}
	    }
	}
        if(pin.x!=QLIMIT && pout->x!=QLIMIT){
                ww=(*weight)*dlen*0.5f;
                if(cfg->debuglevel&dlBary) fprintf(cfg->flog,"barycentric [%f %f %f %f] [%f %f %f %f]\n",
                      bary[0][0],bary[0][1],bary[0][2],bary[0][3],bary[1][0],bary[1][1],bary[1][2],bary[1][3]);

                if(cfg->debuglevel&dlDist) fprintf(cfg->flog,"distances pin-p0: %f p0-pout: %f pin-pout: %f/%f p0-p1: %f\n",
                      dist(&pin,p0),dist(p0,pout),dist(&pin,pout),dist(&pin,p0)+dist(p0,pout)-dist(&pin,pout),dlen);

                for(i=0;i<4;i++)
		     plucker->mesh->weight[ee[i]-1]+=ww*(bary[0][i]+bary[1][i]);
        }
}
