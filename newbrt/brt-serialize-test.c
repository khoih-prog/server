#include "brt-internal.h"

#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>
#include <stdlib.h>

void test_serialize(void) {
    //    struct brt source_brt;
    int nodesize = 1024;
    struct brtnode sn, *dn;
    int fd = open("brt-serialize-test.brt", O_RDWR|O_CREAT, 0777);
    int r;
    const u_int32_t randval = random();
    assert(fd>=0);

    //    source_brt.fd=fd;
    char *hello_string;
    sn.nodesize = nodesize;
    sn.thisnodename = sn.nodesize*20;
    sn.disk_lsn.lsn = 789;
    sn.log_lsn.lsn  = 123456;
    sn.layout_version = 0;
    sn.height = 1;
    sn.rand4fingerprint = randval;
    sn.local_fingerprint = 0;
    sn.u.n.n_children = 2;
    sn.u.n.childkeys[0]    = hello_string = toku_strdup("hello");
    sn.u.n.childkeylens[0] = 6;
    sn.u.n.totalchildkeylens = 6;
    sn.u.n.pivotflags[0] = 42;
    sn.u.n.children[0] = sn.nodesize*30;
    sn.u.n.children[1] = sn.nodesize*35;
    sn.u.n.child_subtree_fingerprints[0] = random();
    sn.u.n.child_subtree_fingerprints[1] = random();
    r = toku_hashtable_create(&sn.u.n.htables[0]); assert(r==0);
    r = toku_hashtable_create(&sn.u.n.htables[1]); assert(r==0);
    r = toku_hash_insert(sn.u.n.htables[0], "a", 2, "aval", 5, BRT_NONE); assert(r==0);    sn.local_fingerprint += randval*toku_calccrc32_cmd(BRT_NONE,   "a", 2, "aval", 5);
    r = toku_hash_insert(sn.u.n.htables[0], "b", 2, "bval", 5, BRT_NONE); assert(r==0);    sn.local_fingerprint += randval*toku_calccrc32_cmd(BRT_NONE,   "b", 2, "bval", 5);
    r = toku_hash_insert(sn.u.n.htables[1], "x", 2, "xval", 5, BRT_NONE); assert(r==0);    sn.local_fingerprint += randval*toku_calccrc32_cmd(BRT_NONE,   "x", 2, "xval", 5);
    sn.u.n.n_bytes_in_hashtables = 3*(BRT_CMD_OVERHEAD+KEY_VALUE_OVERHEAD+2+5);

    toku_serialize_brtnode_to(fd, sn.nodesize*20, sn.nodesize, &sn);  assert(r==0);

    r = toku_deserialize_brtnode_from(fd, nodesize*20, &dn, 0, nodesize, 0, 0, 0, (FILENUM){0});
    assert(r==0);

    assert(dn->thisnodename==nodesize*20);
    assert(dn->disk_lsn.lsn==123456);
    assert(dn->layout_version ==0);
    assert(dn->height == 1);
    assert(dn->rand4fingerprint==randval);
    assert(dn->u.n.n_children==2);
    assert(strcmp(dn->u.n.childkeys[0], "hello")==0);
    assert(dn->u.n.childkeylens[0]==6);
    assert(dn->u.n.totalchildkeylens==6);
    assert(dn->u.n.pivotflags[0]==42);
    assert(dn->u.n.children[0]==nodesize*30);
    assert(dn->u.n.children[1]==nodesize*35);
    {
	int i;
	for (i=0; i<2; i++) {
	    assert(dn->u.n.child_subtree_fingerprints[i]==sn.u.n.child_subtree_fingerprints[i]);
	}
	assert(dn->local_fingerprint==sn.local_fingerprint);
    }
    {
	bytevec data; ITEMLEN datalen; int type;
	r = toku_hash_find(dn->u.n.htables[0], "a", 2, &data, &datalen, &type);
	assert(r==0);
	assert(strcmp(data,"aval")==0);
	assert(datalen==5);
        assert(type == BRT_NONE);

	r=toku_hash_find(dn->u.n.htables[0], "b", 2, &data, &datalen, &type);
	assert(r==0);
	assert(strcmp(data,"bval")==0);
	assert(datalen==5);
        assert(type == BRT_NONE);

	r=toku_hash_find(dn->u.n.htables[1], "x", 2, &data, &datalen, &type);
	assert(r==0);
	assert(strcmp(data,"xval")==0);
	assert(datalen==5);
        assert(type == BRT_NONE);
    }
    brtnode_free(&dn);

    toku_free(hello_string);
    toku_hashtable_free(&sn.u.n.htables[0]);
    toku_hashtable_free(&sn.u.n.htables[1]);
}

int main (int argc __attribute__((__unused__)), char *argv[] __attribute__((__unused__))) {
    memory_check = 1;
    test_serialize();
    malloc_cleanup();
    return 0;
}
