"""Compile the actual Spy command/timer functions with a small server harness."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/spy.c").read_text()
start = source.index("static qboolean SpyCommandCanDisguise")
end = source.index("// Change the Spy's skin", start)
functions = source[start:end]
prefix = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef int qboolean;
typedef intptr_t func_t;
#define true 1
#define false 0
#define PC_SCOUT 1
#define PC_SNIPER 2
#define PC_SOLDIER 3
#define PC_DEMOMAN 4
#define PC_MEDIC 5
#define PC_HVYWEAP 6
#define PC_PYRO 7
#define PC_SPY 8
#define PC_ENGINEER 9
#define EF_DIMLIGHT 8
#define EF_BRIGHTLIGHT 4
#define IT_INVISIBILITY 524288
#define tfset(x) invis_only
#define tfset_cheat_pause 10
#define FOFS(x) 0
typedef struct gedict_s {
 struct { struct { float deadflag,health,effects,items,frame,modelindex,skin,team,nextthink;
 intptr_t owner; func_t think; char *classname; } v; } s;
 int playerclass,isSpectator,team_no,is_unabletospy,is_undercover,StatusBarSize;
 float immune_to_check,undercover_skin,undercover_team,StatusRefreshTime,count,cnt;
} gedict_t;
gedict_t entities[32], *self, *world = entities;
#define EDICT_TO_PROG(e) ((e)-entities)
#define PROG_TO_EDICT(e) (entities+(e))
struct { float time; } g_globalvars;
int invis_only,number_of_teams=2,modelindex_eyes,allied;
char command[64], output[4096];
int allocated, removed[32];
void G_sprint(gedict_t *p,int level,const char *fmt,...) {
 va_list ap; va_start(ap,fmt); vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap); va_end(ap);
}
void G_centerprint(gedict_t *p,const char *fmt,...) {}
void TeamFortress_SetSkin(gedict_t *p) {}
void TeamFortress_SetColor(gedict_t *p,int a,int b) {}
int TeamFortress_TeamGetTopColor(int t) {return t;}
int TeamFortress_TeamGetColor(int t) {return t;}
void UpdateSpyData(gedict_t *p,int t,int s) {}
void TeamFortress_SpyCalcName(gedict_t *p) {}
const char *classes[]={"","Scout","Sniper","Soldier","Demoman","Medic","HWGuy","Pyro","Spy","Engineer"};
void TeamFortress_PrintClassName(gedict_t *p,int c,int x) {G_sprint(p,2,"%s\n",classes[c]);}
int TeamFortress_isTeamsAllied(int a,int b) {return a==b || (allied && b==2);}
void trap_CmdArgv(int a,char *buf,int len) {snprintf(buf,len,"%s",command);}
gedict_t *spawn(void) {assert(allocated<31); return &entities[++allocated];}
void dremove(gedict_t *p) {removed[p-entities]=1;}
gedict_t *trap_find(gedict_t *p,int offset,const char *name) {
 int i; for(i=(int)(p-entities)+1;i<=allocated;i++)
 if(!removed[i] && entities[i].s.v.classname && !strcmp(entities[i].s.v.classname,name)) return &entities[i];
 return NULL;
}
void reset(void) {
 memset(entities,0,sizeof(entities)); memset(removed,0,sizeof(removed)); output[0]=0;
 allocated=1; self=&entities[1]; self->playerclass=PC_SPY; self->team_no=1; self->s.v.health=100;
 invis_only=0; allied=0; number_of_teams=2; g_globalvars.time=100;
}
void TeamFortress_Cmd_Disguise(void);
void TeamFortress_SpyUndercoverThink(void);
void run(const char *name) {self=&entities[1]; snprintf(command,sizeof(command),"%s",name); TeamFortress_Cmd_Disguise();}
void think(int index,float time) {self=&entities[index]; g_globalvars.time=time; TeamFortress_SpyUndercoverThink();}
'''
tests = r'''
int main(void) {
 const char *names[]={"scout","sniper","sold","demo","medic","hwguy","pyro","eng"};
 int ids[]={1,2,3,4,5,6,7,9},i,team; char name[64],expected[1024];
 for(team=1;team<=2;team++) {
  reset(); entities[1].team_no=team; run("disguise_color");
  assert(allocated==2 && entities[2].s.v.nextthink==104);
  assert(!strcmp(output,"Going undercover...\n"));
  think(2,104); assert(entities[1].undercover_team==3-team && removed[2]);
 }
 for(i=0;i<8;i++) {
  reset(); snprintf(name,sizeof(name),"disguise_%s",names[i]); run(name);
  assert(entities[2].s.v.nextthink==104 && !strcmp(output,"Going undercover...\n"));
  think(2,104); assert(entities[1].undercover_skin==ids[i] && !entities[1].undercover_team && removed[2]);
  snprintf(expected,sizeof(expected),"Going undercover...\nSkin set to %s\n",classes[ids[i]]);
  assert(!strcmp(output,expected));
  reset(); snprintf(name,sizeof(name),"disguise_%s_color",names[i]); run(name);
  run(name); assert(allocated==2); /* repeated press cannot overlap timers */
  think(2,104); assert(entities[1].undercover_skin==ids[i] && !entities[1].undercover_team);
  assert(!removed[2] && entities[2].s.v.nextthink==108 && entities[1].is_undercover==2);
  snprintf(expected,sizeof(expected),"Going undercover...\nSkin set to %s\nGoing undercover...\n",classes[ids[i]]);
  assert(!strcmp(output,expected));
  think(2,108); assert(entities[1].undercover_team==2 && removed[2] && entities[1].is_undercover==1);
  strcat(expected,"Colors set to Team 2\n"); assert(!strcmp(output,expected));
 }
 reset(); run("disguise_scout_color"); entities[1].is_undercover=0; think(2,104);
 assert(removed[2] && !entities[1].undercover_skin && !entities[1].undercover_team);
 reset(); run("disguise_scout_color"); think(2,104); entities[1].s.v.deadflag=1; think(2,108);
 assert(removed[2] && !entities[1].undercover_team);
 reset(); run("disguise_scout_color"); entities[1].is_undercover=0; run("disguise_sniper");
 assert(removed[2] && allocated==3); think(3,104); assert(entities[1].undercover_skin==PC_SNIPER);
 reset(); self->playerclass=PC_SCOUT; run("disguise_color"); assert(allocated==1);
 reset(); self->s.v.effects=EF_DIMLIGHT; run("disguise_scout"); assert(allocated==1);
 reset(); self->is_unabletospy=1; run("disguise_scout"); assert(allocated==1);
 reset(); invis_only=1; run("disguise_scout"); assert(allocated==1);
 reset(); number_of_teams=1; run("disguise_scout_color"); assert(allocated==1);
 reset(); number_of_teams=4; allied=1; run("disguise_color"); assert(entities[2].s.v.team==3);
 puts("PASS: 17 commands, both teams, 4+4 second sequence, exact messages, repeat/cancel/death and Spy restrictions");
 return 0;
}
'''
def compile_and_run(source):
    with tempfile.TemporaryDirectory(prefix="tf2003-spy-") as tmp:
        harness = Path(tmp) / "test.c"
        binary = Path(tmp) / "test.exe"
        harness.write_text(source)
        subprocess.run(["gcc", "-std=c99", "-Werror=implicit-function-declaration", str(harness), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    compile_and_run(prefix + functions + tests)
