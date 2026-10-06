"""Compile actual goal-item functions and exercise pickup/drop/return transitions."""
from pathlib import Path
import subprocess
import tempfile

SOURCE = Path(__file__).with_name("tfortmap.c").read_text(encoding="latin1")

def section(start, end):
    return SOURCE[SOURCE.index(start):SOURCE.index(end, SOURCE.index(start))]

functions = section("static void tfgoalitem_StopPhysics", "void item_tfgoal_touch(  )")
functions += section("void tfgoalitem_GiveToPlayer", "void tfgoalitem_TransferToPlayer")
functions += section("void ReturnItem(  )", "void tfgoalitem_RemoveFromPlayer")
functions += section("void tfgoalitem_dropthink", "void tfgoalitem_remove(  )")

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef intptr_t func_t;
typedef struct gedict_s {
 struct {struct {
  intptr_t owner,enemy,groundentity; func_t touch,think;
  int solid,movetype,flags,weapon,effects,items,impulse;
  float nextthink,origin[3],oldorigin[3],mins[3],maxs[3],velocity[3],v_angle[3];
  char *classname,*noise3,*netname;
 } v;} s;
 int goal_state,goal_activation,goal_result,owned_by,team_no,goal_no,playerclass,is_unabletospy;
 float goal_min[3],goal_max[3],camangle[3],camdist,pausetime;
 char *mdl,*noise4;
} gedict_t;
gedict_t ents[4],*world=&ents[0],*self=&ents[2],*other=&ents[3];
struct {float time; float v_forward[3],v_up[3];} g_globalvars;
int CTF_Map,removed,sounds,results,broadcasts;
#define PROG_TO_EDICT(n) (&ents[n])
#define EDICT_TO_PROG(e) ((e)-ents)
#define TFGS_ACTIVE 1
#define TFGS_INACTIVE 2
#define TFGI_GLOW 1
#define TFGI_SLOW 2
#define TFGI_ITEMGLOWS 512
#define TFGI_SOLID 8192
#define TFGR_NO_ITEM_RESULTS 8
#define TFGR_REMOVE_DISGUISE 16
#define PC_SPY 8
#define IT_KEY1 131072
#define IT_KEY2 262144
#define SOLID_NOT 0
#define SOLID_TRIGGER 1
#define SOLID_BBOX 2
#define MOVETYPE_NONE 0
#define MOVETYPE_TOSS 6
#define FL_ONGROUND 512
#define FOFS(x) 0
#define streq(a,b) (!strcmp(a,b))
#define VectorCopy(a,b) memcpy(b,a,sizeof(float)*3)
#define VectorCompare(a,b) (!memcmp(a,b,sizeof(float)*3))
#define VectorScale(a,n,b) do {for(int i=0;i<3;i++) (b)[i]=(a)[i]*(n);} while(0)
#define PASSVEC3(v) (v)[0],(v)[1],(v)[2]
#define SetVector(v,x,y,z) ((v)[0]=x,(v)[1]=y,(v)[2]=z)
#define _R "R"
#define _E "E"
#define _T "T"
#define _U "U"
#define _N "N"
#define _D "D"
#define _B "B"
#define _L "L"
#define _Y "Y"
#define _M "M"
void item_tfgoal_touch(void) {}
void tfgoalitem_remove(void) {}
void setmodel(gedict_t *e,char *s) {} /* MVDSV alias setmodel preserves bounds. */
void setsize(gedict_t *e,float a,float b,float c,float d,float f,float g) {SetVector(e->s.v.mins,a,b,c);SetVector(e->s.v.maxs,d,f,g);}
void setorigin(gedict_t *e,float a,float b,float c) {SetVector(e->s.v.origin,a,b,c);}
void sound(gedict_t *e,int a,char *s,int b,int c) {sounds++;}
void tfgoalitem_checkgoalreturn(gedict_t *e) {results++;}
void UpdateFlagInfoBroadcast(gedict_t *e) {broadcasts++;}
gedict_t *trap_find(gedict_t *e,int a,char *b) {return 0;}
gedict_t *G_NextSpectator(gedict_t *e) {return world;}
void CenterPrint(gedict_t *e,char *fmt,...) {}
void G_bprint(int a,char *fmt,...) {}
void dremove(gedict_t *e) {removed++;}
void DoResults(gedict_t *a,gedict_t *b,int c) {a->goal_state=TFGS_ACTIVE;results++;}
void DoItemGroupWork(gedict_t *a,gedict_t *b) {}
void TeamFortress_SetSpeed(gedict_t *e) {}
int trap_pointcontents(float a,float b,float c) {return -1;}
float g_random(void) {return 0.5f;}
void trap_makevectors(float *v) {SetVector(g_globalvars.v_forward,1,0,0);SetVector(g_globalvars.v_up,0,0,1);}
void aim(float *v) {SetVector(v,1,0,0);}
void SUB_Null(void) {}
'''

tests = r'''
gedict_t *reset(void) {
 memset(ents,0,sizeof(ents));removed=sounds=results=broadcasts=0;
 self=&ents[2];other=&ents[3];self->s.v.enemy=1;self->s.v.weapon=2;
 gedict_t *f=&ents[1];f->s.v.classname="item_tfgoal";f->mdl="progs/tf_stan.mdl";
 SetVector(f->goal_min,-16,-16,-24);SetVector(f->goal_max,16,16,32);
 SetVector(f->s.v.oldorigin,2392,-1192,400);SetVector(f->s.v.origin,2898,-714,264);
 SetVector(f->s.v.mins,0,0,-24);SetVector(f->s.v.maxs,0,0,-24);
 SetVector(f->s.v.velocity,10,20,400);f->s.v.think=123;f->s.v.nextthink=336;
 f->s.v.flags=FL_ONGROUND|256;f->s.v.groundentity=3;f->s.v.owner=3;
 f->s.v.movetype=MOVETYPE_TOSS;f->pausetime=30;g_globalvars.time=100;
 return f;
}
void stopped(gedict_t *f) {
 assert(f->s.v.movetype==MOVETYPE_NONE && !f->s.v.think && !f->s.v.nextthink);
 assert(VectorCompare(f->s.v.velocity,world->s.v.velocity));
 assert(!(f->s.v.flags&FL_ONGROUND) && (f->s.v.flags&256));
 assert(f->s.v.groundentity==0);
}
void returned(gedict_t *f,int solid) {
 stopped(f);assert(f->s.v.owner==0 && f->s.v.solid==solid);
 assert(f->goal_state==TFGS_INACTIVE);
 assert(VectorCompare(f->s.v.origin,f->s.v.oldorigin));
 assert(VectorCompare(f->s.v.mins,f->goal_min) && VectorCompare(f->s.v.maxs,f->goal_max));
 assert(sounds==1);
}
int main(void) {
 gedict_t *f;
 for(int solid=0;solid<2;solid++) {
  f=reset();f->goal_activation=solid?TFGI_SOLID:0;ReturnItem();
  returned(f,solid?SOLID_BBOX:SOLID_TRIGGER);assert(results==1 && removed==1 && broadcasts==1);
 }
 f=reset();f->goal_min[0]=-8;f->goal_max[2]=48;ReturnItem();returned(f,SOLID_TRIGGER);
 f=reset();tfgoalitem_GiveToPlayer(f,other,f);stopped(f);
 assert(f->s.v.owner==3 && f->s.v.solid==SOLID_NOT && f->goal_state==TFGS_ACTIVE);
 f=reset();ents[2].goal_result=TFGR_NO_ITEM_RESULTS;tfgoalitem_GiveToPlayer(f,other,self);
 stopped(f);assert(f->goal_state==TFGS_ACTIVE && results==0 && broadcasts==1);
 f=reset();f->s.v.items=IT_KEY2;f->goal_activation=TFGI_GLOW|TFGI_SLOW;
 f->goal_result=TFGR_REMOVE_DISGUISE;other->playerclass=PC_SPY;
 tfgoalitem_GiveToPlayer(f,other,f);assert(other->s.v.items&IT_KEY2);
 assert(other->s.v.effects&8);assert(other->is_unabletospy);
 f=reset();CTF_Map=1;self=f;f->goal_no=2;other->team_no=2;
 item_tfgoal_touch_CTF();returned(f,SOLID_TRIGGER);
 f=reset();CTF_Map=1;self=f;f->goal_no=2;other->team_no=1;
 item_tfgoal_touch_CTF();assert(f->s.v.nextthink==336 && !sounds);
 f=reset();CTF_Map=0;self=f;item_tfgoal_touch_CTF();assert(!sounds);
 /* Death drop retains the original .75 + pausetime return schedule. */
 f=reset();tfgoalitem_GiveToPlayer(f,other,f);tfgoalitem_drop(f,0,other);
 assert(f->s.v.movetype==MOVETYPE_TOSS && f->s.v.think==(func_t)tfgoalitem_dropthink);
 assert(f->s.v.nextthink==100.75 && VectorCompare(f->s.v.mins,f->goal_min));
 self=f;g_globalvars.time=100.75;tfgoalitem_dropthink();
 assert(f->s.v.think==(func_t)tfgoalitem_remove && f->s.v.nextthink==130.75);
 /* Intentional drop retains its .75 + 4.25 + pausetime schedule. */
 f=reset();tfgoalitem_drop(f,1,other);assert(f->s.v.touch==(func_t)SUB_Null);
 self=f;g_globalvars.time=100.75;tfgoalitem_droptouch();assert(f->s.v.nextthink==105);
 g_globalvars.time=105;tfgoalitem_dropthink();assert(f->s.v.nextthink==135);
 puts("PASS: crushed/custom bounds, standard/solid/CTF returns, ownership, pickup timers/physics, Spy/key/glow effects, unchanged drop delays");
}
'''

if __name__ == "__main__":
    scratch = Path(__file__).with_name("tmp")
    scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="tf2003-goalitem-", dir=scratch) as tmp:
        c = Path(tmp) / "test.c"
        exe = Path(tmp) / "test.exe"
        c.write_text(prefix + functions + tests, encoding="ascii")
        subprocess.run(["gcc", "-std=c99", "-Werror=implicit-function-declaration", str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
