#include "common.h"

struct credit_allocator *CA;
//id for vif
int vif_cnt;
/*
//proc
static struct proc_dir_entry *proc_root_dir;
static struct proc_dir_vif proc_vif[20];
static int idx;
int fileread = 0;
*/

static void credit_accounting(unsigned long data){
	struct ancs_container *temp_vif, *next_vif;
	int total = CA->total_weight;
	unsigned int weight_left =total;
	unsigned int credit_left = 0;
	unsigned int credit_total = MAX_CREDIT;
	unsigned int credit_fair;
	int credit_xtra =0;

	temp_vif=next_vif = NULL;
	
//	printk(KERN_INFO "calling credit accounting function\n");
	if(list_empty(&CA->active_vif_list) || total ==0)	
		goto out;
	if(CA->credit_balance > 0)
		credit_total += CA->credit_balance;

	list_for_each_entry_safe(temp_vif, next_vif, &CA->active_vif_list, vif_list){
		if(!temp_vif)	goto out;
		
		weight_left -= temp_vif->weight;
		credit_fair = ((credit_total * temp_vif->weight) + (total -1 ) / total);
		temp_vif->remaining_credit = credit_fair;
		
		if(temp_vif->min_credit!=0 || temp_vif->max_credit!=0){
			if(temp_vif->min_credit!=0 && temp_vif->remaining_credit < temp_vif->min_credit){
				credit_total-=(temp_vif->min_credit - temp_vif->remaining_credit);
				temp_vif->remaining_credit = temp_vif->min_credit;
				total-=temp_vif->weight;
				list_del(&temp_vif->vif_list);
				list_add(&temp_vif->vif_list, &CA->active_vif_list);
			}
			else if(temp_vif->max_credit!=0 && temp_vif->remaining_credit > temp_vif->max_credit){
				credit_total+= (temp_vif->remaining_credit - temp_vif->max_credit);
				temp_vif->remaining_credit = temp_vif->max_credit;
				total-=temp_vif->weight;
				list_del(&temp_vif->vif_list);
				list_add(&temp_vif->vif_list, &CA->active_vif_list);
			}
			goto skip;
		}
		if(temp_vif->used_credit != 0){
			credit_xtra = 1;
		}
		else
		{
			credit_left += (temp_vif->remaining_credit - credit_fair);

			if(weight_left != 0U){
				credit_total += ((credit_left*total)+(weight_left - 1))/weight_left;
				credit_left=0;
			}
			if(credit_xtra){
				list_del(&temp_vif->vif_list);
				list_add(&temp_vif->vif_list, &CA->active_vif_list);
			}
			
			if(CA->num_vif > 1)
				temp_vif->remaining_credit = credit_fair;
			else
				temp_vif->remaining_credit = MAX_CREDIT;
		}
skip:	
	if(temp_vif->need_reschedule == true)
		temp_vif->need_reschedule = false;
	}
	printk("credit distribution - vif_id:%d, credit:%u\n", temp_vif->id, temp_vif->remaining_credit);
	CA->credit_balance = credit_left;
	
out:
//	spin_unlock(&netbk->active_vif_list_lock);
	mod_timer(&CA->account_timer, jiffies + msecs_to_jiffies(30));
	return;
}
/*
static ssize_t vif_write(struct file *file, const char __user* user_buffer, size_t count, loff_t *ppos)
{
        char* filename = file->f_path.dentry->d_name.name;
	char* input = strsep(&user_buffer,"\n");
       struct ancs_vm *vif;
	char *endptr;
        unsigned int value = simple_strtol(input, &endptr, 10);
        
        if(endptr == input && *endptr != '\0')
        {
                printk("invalid input!\n");
                return count;
        }

	vif = PDE_DATA(file_inode(file));	
	if(!(vif)){
		printk(KERN_INFO "NULL Data\n");
		return 0;
	}

	if(!strcmp(filename, "min_credit"))
        {
		vif->min_credit = value;
		goto proc_out;
        }
	
	if(!strcmp(filename, "max_credit"))
        {
		vif->max_credit = value;
		goto proc_out;
        }

	if(!strcmp(filename, "weight"))
        {
		vif->weight = value;
		goto proc_out;
        }
	return count;
	
}

static ssize_t vif_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char* filename = file->f_path.dentry->d_name.name;
	struct ancs_vm *vif;	
	unsigned int len;
	
	vif = PDE_DATA(file_inode(file));
	if(!(vif)){
                printk(KERN_INFO "NULL Data\n");
                return 0;
        }
	if(!strcmp(filename, "min_credit")){
		len = sprintf(buf, "%d\n", vif->min_credit);
		goto proc_out;
		}
	else if(!strcmp(filename, "max_credit")){
		len = sprintf(buf, "%d\n", vif->max_credit);
		goto proc_out;
		}
	else if(!strcmp(filename, "weight")){
		len = sprintf(buf, "%d\n", vif->weight);
		goto proc_out;
		}
	else if(!strcmp(filename, "remaining_credit")){
		len = sprintf(buf, "%d\n", vif->remaining_credit);
		goto proc_out;
		}
	else if(!strcmp(filename, "used_credit")){
       	len = sprintf(buf, "%d\n", vif->used_credit);
		goto proc_out;
		}
	else if(!strcmp(filename, "pid")){
		len = sprintf(buf, "%d\n", vif->vhost->pid);
		goto proc_out;
		}
	else{
		count = sprintf(buf, "%s", "ERROR");
		return count;
		}

proc_out:
	if(fileread == 0){
                        fileread = 1;
                        return len;
                }
                else
                {
                        fileread = 0;
                        return 0;
                }
}

static const struct file_operations vif_opt ={
	.write = vif_write,
	.read = vif_read,
};
*/
int pay_credit(struct net_bridge_port *p, unsigned int packet_data_size){
	//if date_len is zero then it means no fragment
	printk(KERN_INFO "MINKOO:vif%u remaining credit:%u paying:%u", p->vif->id, p->vif->remaining_credit, packet_data_size);
	if(p->vif->remaining_credit == ~0U){
		printk(KERN_INFO "PAYMENT SUCCESS\n");
		return PAY_SUCCESS;
	}
	if(p->vif->remaining_credit < packet_data_size){
		printk(KERN_INFO "PAYMENT FAILURE\n");
		return PAY_FAIL;
	}
	else{
		p->vif->remaining_credit -= packet_data_size;
		printk(KERN_INFO "PAYMENT SUCCESS\n");
		return PAY_SUCCESS;
	}
	return PAY_SUCCESS;
}

void new_vif(struct net_bridge_port *p){
	if(p==NULL){
		printk(KERN_ERR "MINKOO: new port pointer null err\n");
		return;
	}
	
	
	//initialize new ancs_container (p->vif)
	struct ancs_container *vif;
	vif = kmalloc(sizeof(struct ancs_container), GFP_KERNEL | __GFP_NOWARN | __GFP_REPEAT);
	INIT_LIST_HEAD(&vif->vif_list);
	vif->need_reschedule = false;
	vif->weight = vif_cnt;	//0 is arbitary value, give weight to check QOS algorithm working.
	vif->min_credit = 0;		//arbitary
	vif->max_credit = 0;		//arbitary
	vif->remaining_credit = ~0U; 
	vif->used_credit = 0;	
	vif->id = vif_cnt++;
	
	//add vif to credit allocator list
	list_add(&vif->vif_list, &CA->active_vif_list);
	
	//update function for credit allocator
	update_CA(vif, PLUS);
	
	p->vif = vif;	

	printk(KERN_INFO "MINKOO: new vif%d\n", p->vif->id);
}

void del_vif(struct net_bridge_port *p){
	if(p==NULL){
		printk(KERN_ERR "MINKOO: del port pointer null err\n");
                return;
	}
	else if(p->vif == NULL){
		printk(KERN_ERR "MINKOO: del vif pointer null err\n");
                return;
	}
	else printk(KERN_INFO "MINKOO: delete vif%d\n", p->vif->id);
	
	//delete list from credit allocator
	list_del(&p->vif->vif_list);
	
	//update function for credit allocator
	update_CA(p->vif, MINUS);
	
	//free memory
	kfree(p->vif);
}

void update_CA(struct ancs_container *vif, int isplus){
	if(isplus == PLUS){
		CA->total_weight += vif->weight;
		CA->num_vif++;
	}
	if(isplus == MINUS){
		CA->total_weight -= vif->weight;
		CA->num_vif--;
	}
}

static int __init vif_init(void)
{
	//variables
	int cpu = smp_processor_id();
	vif_cnt = 1;	

	//function pointer linking
	fp_pay = &pay_credit;
	fp_newvif = &new_vif;
	fp_delvif = &del_vif;	

	//credit allocator initialization
	CA = kmalloc(sizeof(struct credit_allocator), GFP_KERNEL | __GFP_NOWARN | __GFP_REPEAT);
	if (!CA)
		return -ENOMEM;	
	CA->total_weight = 0;
	CA->credit_balance = 0;
	CA->num_vif =0;
	INIT_LIST_HEAD(&CA->active_vif_list);
	spin_lock_init(&CA->active_vif_list_lock);

	//setting up timer for callback function
	setup_timer(&CA->account_timer, credit_accounting, cpu );
	mod_timer(&CA->account_timer, jiffies + msecs_to_jiffies(30));

	printk(KERN_INFO "MINKOO: credit allocator init!!\n");	

	return 0;
}

static void __exit vif_exit(void)
{
	printk(KERN_INFO "MINKOO: credit allocator exit!!\n");
	
	//free CA
	kfree(CA);
	
	//delete timer
	del_timer(&CA->account_timer);
	
	return;
}

module_init(vif_init);
module_exit(vif_exit);

MODULE_AUTHOR("Korea University");
MODULE_DESCRIPTION("OSLAB");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("ver 1.0");
