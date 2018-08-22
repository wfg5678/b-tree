/*
A simple b-tree implementation. All nodes are held in memory. Each node consists of space for 2t keys. The keys are integers from 0 to 2^31-1. The node also has 2t+1 pointers to other nodes. This implementation follows the CLRS convention where t is the minimum degree. Every node (except the root) must have at least t-1 keys and no node can have more than 2t keys.

Supports:

Creating a B-tree
Adding key to B-tree
Deleting key from B-tree
Searching for key in B-tree
Destroying B-tree

*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define t 3

struct bt_node{

  int count;
  int isLeaf;
  int keys[2*t];
  struct bt_node* children[2*t+1];
};



//====================General Functions==============================

void print_tree(struct bt_node* node, int level);
struct bt_node* create_b_tree();
void destroy_b_tree(struct bt_node** ref_head);
void free_node(struct bt_node* node);
int check_key(struct bt_node* head, int to_check);

//====================Functions for Adding Key=======================

void add_key(struct bt_node** ref_head, int to_add);
void find_and_insert_key(struct bt_node* node, int to_add);
int find_position(struct bt_node* node, int to_add);
void shift_over(struct bt_node* node, int index);
void insert_in_leaf(struct bt_node* leaf, int to_add);
void insert_in_internal(struct bt_node* node, int key);
struct bt_node* create_sibling(struct bt_node* full);
void copy(struct bt_node* full, struct bt_node* new_sibling);
struct bt_node* split_node();


//====================Functions for Deleting Key=====================
int remove_key(struct bt_node* node, int to_check, struct bt_node** ref_head);
void remove_from_leaf(struct bt_node* leaf, int index);
void get_replacement(struct bt_node* node, int index);
int ensure_enough_keys(struct bt_node* parent, int index, struct bt_node** ref_head);
void steal_key_left(struct bt_node* parent, int index);
void steal_key_right(struct bt_node* parent, int index);
int get_previous_key(struct bt_node* node);
int get_next_key(struct bt_node* node);
int consolidate(struct bt_node* parent, int index, struct bt_node** ref_head);

void print_tree(struct bt_node* node, int level){

  if(node->isLeaf == 1){
    printf("\n");
    for(int i=0; i<level; i++){
      printf("\t");
    }
    for(int i=0; i<node->count; i++){
      printf("%d ", node->keys[i]);
    }
  }
  else{
    print_tree(node->children[0], level+1);
    for(int i=0; i<node->count; i++){
      
      printf("\n");
      for(int i=0; i<level; i++){
	printf("\t");
      }
      printf("%d ", node->keys[i]);
      print_tree(node->children[i+1], level+1);
    }
  }
  return;
}


struct bt_node* create_b_tree(){

  struct bt_node* head = malloc(sizeof(struct bt_node));
  head->count = 0;
  head->isLeaf = 1;
  return head;
}

void destroy_b_tree(struct bt_node** ref_head){

  if((*ref_head)->isLeaf == 0){
    for(int i=0; i<=(*ref_head)->count; i++){
      free_node((*ref_head)->children[i]);
    }
  }

  free((*ref_head));
  (*ref_head) = NULL;
  return;
}

/*
  Recursively step through children of 'node' and free them and their
  children. Then free 'node'
*/
void free_node(struct bt_node* node){

  if(node->isLeaf == 0){

    for(int i=0; i<=node->count; i++){
      free_node(node->children[i]);
    }
  }
  free(node);
  return;
}

/*
  Searches b-tree for key 'to_check'. Returns 1 if key exist in the
  tree. Otherwise, return 0.
*/
int check_key(struct bt_node* head, int to_check){

  struct bt_node* node = head;
  int index;

  while(1){

    index = 0;
    while(index < node->count && to_check >= node->keys[index]){

      if(to_check == node->keys[index]){
	return 1;
      }
      index++;
    }
    if(node->isLeaf == 0){
      node = node->children[index];
    }
    else{
      return 0;
    }
  }
}

/*
  First check to see if root node is full.
  If so split it and add another node on top of the tree
  Then call the find_and_insert recursive function
*/
void add_key(struct bt_node** ref_head, int to_add){

  struct bt_node* node = (*ref_head);

  //first have to make sure that the root node is not full
  if(node->count == 2*t){
    struct bt_node* new_head = malloc(sizeof(struct bt_node));

    (*ref_head) = new_head;

    new_head->count = 0;
    new_head->isLeaf = 0;

    new_head->children[0] = node;
    node = split_node(new_head, 0, to_add);
    
  }

  find_and_insert_key(node, to_add);
  return;
}

/*
  Checks to see if 'node' is a leaf. If so insert key 'to_add' in
  proper place. Otherwise, find child that should hold 'to_add' and
  recursively call 'find_and_insert_key().
*/
void find_and_insert_key(struct bt_node* node, int to_add){

  if(node->isLeaf == 1){
    insert_in_leaf(node, to_add);
    return;
  }
  else{
    int index = find_position(node, to_add);
    struct bt_node* child = node->children[index];

    if(child->count == 2*t){
      child = split_node(node, index, to_add);
    }

    find_and_insert_key(child, to_add);
  }

  return;
}

/*
  Find and return the index of 'node->children[]' that should hold
  'to_add'
*/
int find_position(struct bt_node* node, int to_add){

  for(int i=0; i<node->count; i++){
    if(to_add < node->keys[i]){
      return i;
    }
  }
  return node->count;
}
  
//shift keys over to make room for a new key
void shift_over(struct bt_node* node, int index){

  for(int i = node->count; i>index; i--){
    node->keys[i] = node->keys[i-1];
  }

  return;
}

/*
  Inserts a key in the proper place and then shifts all other keys
  after rightward one spot.
*/
void insert_in_leaf(struct bt_node* leaf, int to_add){

  int index = find_position(leaf, to_add);
  shift_over(leaf, index);
  leaf->keys[index] = to_add;
  leaf->count++;
  return;
}				     

//Same as insert_in_leaf() but also moves pointers to children down
void insert_in_internal(struct bt_node* node, int key){

  int index = find_position(node, key);
  shift_over(node, index);
  node->keys[index] = key;
  node->count++;

  for(int i=node->count+1; i>index; i--){
    node->children[i] = node->children[i-1];
  } 
  
  return;
}		
  
struct bt_node* create_sibling(struct bt_node* full){

  struct bt_node* new_sibling = malloc(sizeof(struct bt_node));
  if(new_sibling == NULL){
    printf("error in malloc\n");
  }
  //set leaf property
  new_sibling->isLeaf = full->isLeaf;
  
  return new_sibling;
}

/*The keys  stay in full.
  The keys after len/2 move to new_sibling
  If nodes are internal node the pointers to children are modified as
  well.
*/
void copy(struct bt_node* full, struct bt_node* new_sibling){

  int index = 0;
  for(int i=t+1; i<2*t; i++){
    new_sibling->keys[index] = full->keys[i];
    index++;
  }
  
  if(full->isLeaf == 0){
    index = 0;
    for(int i=t+1; i<=2*t; i++){
      new_sibling->children[index] = full->children[i];
      index++;
    }
  }
  return;
}


struct bt_node* split_node(struct bt_node* parent, int index, int to_add){

  struct bt_node* full = parent->children[index];
  
  int median_key = full->keys[t];

  insert_in_internal(parent, median_key);

  struct bt_node* new_child = create_sibling(full);
  copy(full, new_child);
    
  new_child->count = t-1;
  full->count = t;

  parent->children[index+1] = new_child;

  
  if(to_add < new_child->keys[0]){
    return full;
  }
  else{
    return new_child;
  }

}

int remove_key(struct bt_node* node, int to_check, struct bt_node** ref_head){

  int index = 0;

  while(index < node->count && to_check > node->keys[index]){
    index++;
  }
 
  if(node->isLeaf == 1){
    
    if(to_check == node->keys[index]){
      remove_from_leaf(node, index);
      return 1;
    }
    else{
      return 0;
    }
  }
  else{
    if(to_check == node->keys[index]){

      if(node->children[index]->count >= t){

	struct bt_node* left_child = node->children[index];
	to_check = get_previous_key(left_child);
	node->keys[index] = to_check;
	node = left_child;
      }
      else if(node->children[index+1]->count >= t){
	
	struct bt_node* right_child = node->children[index+1];
	to_check = get_next_key(right_child);
	node->keys[index] = to_check;
	node = right_child;
      }
      else{

	index = consolidate(node, index, ref_head);
	node = node->children[index];
      }
      return remove_key(node, to_check, ref_head);
    }
    else{
      index = ensure_enough_keys(node, index, ref_head);
      return remove_key(node->children[index], to_check, ref_head);
    }
  }

}      


//remove key at 'index' by shifting the keys to the left
void remove_from_leaf(struct bt_node* leaf, int index){

  for(int i=index; i<leaf->count; i++){
    leaf->keys[i] = leaf->keys[i+1];
  }
  leaf->count--;
  return;
}

void get_replacement(struct bt_node* node, int index){

  struct bt_node* left_child = node->children[index];
  int replacement = left_child->keys[left_child->count-1];

  left_child->keys[left_child->count-1] = node->keys[index];
  node->keys[index] = replacement;

  return;
}

/*
  Makes sure that there is at least t  keys in the child at 
  'parent->children[index]. This ensures that a key can be remove from
  the child if necessary.
  If there are less than t keys there are a couple of ways to make up 
  for the deficit.
  1. Try to steal a key from the left neighbor if it exists
  2. Try to steal from the right neighbor
  3. Consolidate the child node and one its neighbors into a single 
  node.
*/
int ensure_enough_keys(struct bt_node* parent, int index, struct bt_node** ref_head){

  struct bt_node* child = parent->children[index];

  if(child == NULL)
  
  if(child->count < t){
    steal_key_left(parent, index);
  }

  if(child->count < t){
    steal_key_right(parent, index);
  }

  if(child->count < t){
    index = consolidate(parent, index, ref_head);
  }
 
  return index;
}

void steal_key_left(struct bt_node* parent, int index){

  struct bt_node* child = parent->children[index];
  
  //there is no left sibling to steal from if index=0
  //left most child of parent
  if(index != 0){

    struct bt_node* left_sibling = parent->children[index-1];
    if(left_sibling->count >= t){

      for(int i=child->count; i>=1; i--){

	child->keys[i] = child->keys[i-1];
      }
      for(int i=child->count+1; i>=1; i--){

	child->children[i] = child->children[i-1];
      }
      child->count++;
      child->keys[0] = parent->keys[index-1];
      child->children[0] = left_sibling->children[left_sibling->count];

      parent->keys[index-1] = left_sibling->keys[left_sibling->count-1];

      left_sibling->children[left_sibling->count] = NULL; //really shouldnt be necessary
      left_sibling->count--;
      return;     
    }
  }

  return;
}

void steal_key_right(struct bt_node* parent, int index){

  struct bt_node* child = parent->children[index];
  
  //there is no right sibling to steal from if index=parent->count
  //right most child of parent
  if(index != parent->count){

    struct bt_node* right_sibling = parent->children[index+1];

    if(right_sibling->count >= t){

      //move parent to child
      child->keys[child->count] = parent->keys[index];
      child->children[child->count+1] = right_sibling->children[0];
      child->count++;

      //move right_sibling to parent
      parent->keys[index] = right_sibling->keys[0];

      //shift 'right_sibling' down to fill in gap
      
      for(int i=1; i<right_sibling->count; i++){

	right_sibling->keys[i-1] = right_sibling->keys[i];
      }
      for(int i=1; i<=right_sibling->count; i++){

	right_sibling->children[i-1] = right_sibling->children[i];
      }

      right_sibling->children[right_sibling->count] = NULL; //really shouldnt be necessary
      right_sibling->count--;
      return;     
    }
  }

  return;
}

int get_previous_key(struct bt_node* node){

  while(node->isLeaf == 0){
    node = node->children[node->count];
  }
  return node->keys[node->count-1];
}

int get_next_key(struct bt_node* node){

  while(node->isLeaf == 0){
    node = node->children[0];
  }
  return node->keys[0];
}


int consolidate(struct bt_node* parent, int index, struct bt_node** ref_head){

  if(index != parent->count){
    struct bt_node* left = parent->children[index];
    struct bt_node* right = parent->children[index+1];

    int count = left->count;

    left->keys[count] = parent->keys[index];
    count++;

    //transfer keys from right to left
    for(int i=0; i<right->count; i++){

      left->keys[i+count] = right->keys[i];
    }

    //transer pointers to children from right to left
    for(int i=0; i<=right->count; i++){

      left->children[i+count] = right->children[i];
    }

    left->count = left->count + right->count+1;
    free(right);

    
    //move 'parent' keys and children over to fill gap
    for(int i=index+1; i<parent->count; i++){
      parent->keys[i-1] = parent->keys[i];
    }
    for(int i=index+1; i<=parent->count; i++){
      parent->children[i] = parent->children[i+1];
    }
    parent->count--;
  }
  else{
    struct bt_node* left = parent->children[index-1];
    struct bt_node* right = parent->children[index];

    int count = left->count;

    left->keys[count] = parent->keys[index-1];
    count++;

    //transfer keys from right to left
    for(int i=0; i<right->count; i++){

      left->keys[i+count] = right->keys[i];
    }

    //transer pointers to children from right to left
    for(int i=0; i<=right->count; i++){

      left->children[i+count] = right->children[i];
    }

    left->count = left->count + right->count+1;
    free(right);

    parent->count--;
    index--;
  }

  if(parent->count == 0){
    (*ref_head) = parent->children[0];
    free(parent);
  }
  return index;
}

int get_selection_option(){

  printf("\n\nNEXT ACTION (1 = ADD, 2 = DELETE, 3 = QUIT)\n");
  char num[32];
  fgets(num,32,stdin);
  
  int total = num[0]-'0';
  int index=1;
  while(num[index] != '\n'){
    total = total*10 + (num[index] - '0');
    index++;
  }

  if(total == 1){
    return 1;
  }
  if(total == 2){
    return 2;
  }

  return 3;
}

void get_key_to_add(struct bt_node** ref_head){

  printf("KEY TO ADD: ");
   char num[32];
  fgets(num,32,stdin);
  
  int total = num[0]-'0';
  int index=1;
  while(num[index] != '\n'){
    total = total*10 + (num[index] - '0');
    index++;
  }

  if(total >= 0 && total < 2147483647){
    add_key(ref_head, total);
    printf("\nADDED %d\n", total);
  }
  else{
    printf("\nNOT VALID INPUT\n");
  }
  return;
}

void get_key_to_delete(struct bt_node** ref_head){

  printf("KEY TO DELETE: ");
   char num[32];
  fgets(num,32,stdin);
  
  int total = num[0]-'0';
  int index=1;
  while(num[index] != '\n'){
    total = total*10 + (num[index] - '0');
    index++;
  }

  if(total >= 0 && total < 2147483647){
 
    if(remove_key((*ref_head), total, ref_head) == 1){
      printf("\nDELETING %d\n", total);
    }
    else{
      printf("\n%d NOT IN TREE\n", total);
    }
  }
  else{
    printf("\nNOT VALID INPUT\n");
  }
  return;
}


 
int main(){

  srand(time(NULL));

  struct bt_node* head = create_b_tree();

  int option;
  while(1){

    option = get_selection_option();
    if(option == 1){
      get_key_to_add(&head);
      print_tree(head, 0);
    }
    else if(option == 2){
      get_key_to_delete(&head);
      print_tree(head, 0);
    }
    else{
      break;
    }
  }

  destroy_b_tree(&head);



   return 0;
}




/*
void remove_key(struct bt_node* node, int to_check, struct bt_node** ref_head){

  int index = 0;

  while(index < node->count && to_check > node->keys[index]){

    index++;
  }

  if(to_check == node->keys[index]){

    if(node->isLeaf == 1){
      remove_from_leaf(node, index);
      return;
    }
    else{

      if(node->children[index]->count >= t){

	struct bt_node* left_child = node->children[index];
	to_check = get_previous_key(left_child);
	node->keys[index] = to_check;
	node = left_child;
      }
      else if(node->children[index+1]->count >= t){
	
	struct bt_node* right_child = node->children[index+1];
	to_check = get_next_key(right_child);
	node->keys[index] = to_check;
	node = right_child;
      }
      else{

	index = consolidate(node, index, ref_head);
	node = node->children[index];
      }
      remove_key(node, to_check, ref_head);

    }
    
  }
  else{
    index = ensure_enough_keys(node, index, ref_head);
    remove_key(node->children[index], to_check, ref_head);
  }
  return;
}

*/
