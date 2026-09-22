/*
 * main.c - 简化版 Linux 红黑树的测试用例
 *
 * 参考官方文档: https://docs.kernel.org/translations/zh_CN/core-api/rbtree.html
 *
 * 用户只需要自己实现 search() 和 insert() 两个函数,
 * 删除(rb_erase)、遍历(rb_first/rb_next)等由库直接提供。
 * 编译: make
 * 运行: make run 或 ./rbtree_test
 */

#include <stddef.h>
#include <stdio.h>

#include "rbtree.h"

/*
 * 测试节点: 内嵌 struct rb_node, 另外只有一个 int 成员 key,
 * 红黑树按 key 的大小排序。
 */
struct my_node {
	struct rb_node node;
	int key;
};

/* 统一的结果检查宏, 通过则打印 [PASS], 失败则打印 [FAIL] 并计数 */
#define CHECK(cond, pass_msg, fail_msg)                                 \
	do {                                                            \
		if (cond) {                                             \
			printf("  [PASS] %s\n", pass_msg);              \
		} else {                                                \
			printf("  [FAIL] %s\n", fail_msg);              \
			failures++;                                     \
		}                                                       \
	} while (0)

/*
 * 搜索: 在树中查找 key 对应的节点, 找不到返回 NULL。
 * 这是需要自己实现的函数之一。
 */
struct my_node *search(struct rb_root *root, int key)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct my_node *data = rb_entry(node, struct my_node, node);

		if (key < data->key)
			node = node->rb_left;
		else if (key > data->key)
			node = node->rb_right;
		else
			return data;	/* 找到了 */
	}
	return NULL;
}

/*
 * 插入: 按 key 大小插入新节点, 并调用 rb_insert_color() 重新平衡。
 * 返回 0 表示成功, 返回 -1 表示 key 重复 (不插入)。
 * 这是需要自己实现的函数之二。
 */
int insert(struct rb_root *root, struct my_node *data)
{
	struct rb_node **new = &root->rb_node, *parent = NULL;

	/* 找到合适的插入位置 */
	while (*new) {
		struct my_node *this = rb_entry(*new, struct my_node, node);

		parent = *new;
		if (data->key < this->key)
			new = &(*new)->rb_left;
		else if (data->key > this->key)
			new = &(*new)->rb_right;
		else
			return -1;	/* 重复 key */
	}

	/* 先链接成红色叶子, 再调用库函数修正颜色/旋转 */
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);

	return 0;
}

/* 中序遍历整棵树 (rb_first + rb_next), 同时校验是否严格升序, 返回节点数 */
static int print_inorder(struct rb_root *root)
{
	struct rb_node *node;
	int count = 0, prev = 0, sorted = 1;

	for (node = rb_first(root); node; node = rb_next(node)) {
		struct my_node *data = rb_entry(node, struct my_node, node);

		if (count > 0 && data->key <= prev)
			sorted = 0;
		prev = data->key;
		printf("%d ", data->key);
		count++;
	}
	printf("\n");
	if (!sorted)
		printf("  警告: 中序遍历结果不是升序!\n");
	return count;
}

/*
 * 校验红黑树性质: 递归返回子树黑高, 不合法返回 -1。
 * 规则: 不能出现两个连续红节点; 左右子树黑高相等。
 */
static int check_properties(struct rb_node *node, struct rb_node *parent)
{
	int lh, rh;

	if (!node)
		return 1;	/* NULL 叶子视为黑色, 黑高为 1 */

	/* 红节点的孩子不能是红色 */
	if (parent && rb_is_red(node) && rb_is_red(parent))
		return -1;

	lh = check_properties(node->rb_left, node);
	rh = check_properties(node->rb_right, node);
	if (lh == -1 || rh == -1 || lh != rh)
		return -1;	/* 左右黑高必须相等 */

	return lh + (rb_is_black(node) ? 1 : 0);
}

/* 校验整棵树: 根必须是黑色, 且满足红黑树性质 */
static int verify(struct rb_root *root)
{
	if (!root->rb_node)
		return 1;
	if (rb_is_red(root->rb_node))
		return 0;
	return check_properties(root->rb_node, NULL) > 0;
}

/* 计算树中节点总数 */
static int count_nodes(struct rb_root *root)
{
	struct rb_node *node;
	int count = 0;

	for (node = rb_first(root); node; node = rb_next(node))
		count++;
	return count;
}

int main(void)
{
	struct rb_root root = RB_ROOT;
	/* 1~20 打乱顺序插入, 以便触发各种旋转/变色分支 */
	int keys[] = {15, 3, 8, 20, 1, 12, 6, 18, 10, 5,
		      2, 14, 9, 17, 4, 19, 7, 11, 13, 16};
	int n = sizeof(keys) / sizeof(keys[0]);
	static struct my_node nodes[sizeof(keys) / sizeof(keys[0])];
	struct my_node dup;
	int i, failures = 0;

	printf("================ 1. 插入测试 ================\n");
	for (i = 0; i < n; i++) {
		nodes[i].key = keys[i];
		if (insert(&root, &nodes[i]) == 0)
			printf("  insert key=%2d -> ok\n", keys[i]);
		else {
			printf("  insert key=%2d -> 失败!\n", keys[i]);
			failures++;
		}
	}
	printf("  插入 %d 个节点后中序遍历: ", n);
	print_inorder(&root);
	CHECK(verify(&root), "插入后红黑树性质校验通过",
	      "插入后红黑树性质校验失败!");

	/* 重复 key 应该被拒绝 */
	dup.key = 10;
	{
		int r = insert(&root, &dup);

		printf("  再次插入重复的 key=10 -> %s\n",
		       r == -1 ? "被拒绝" : "竟然成功了!");
		CHECK(r == -1, "重复 key 被正确拒绝", "重复 key 没有被拒绝!");
	}

	printf("\n================ 2. 搜索测试 ================\n");
	{
		int miss = 0;

		for (i = 0; i < n; i++) {
			struct my_node *data = search(&root, keys[i]);

			printf("  search key=%2d -> %s\n", keys[i],
			       (data && data->key == keys[i]) ?
				       "命中" : "未命中!");
			if (!data || data->key != keys[i])
				miss++;
		}
		CHECK(miss == 0, "全部 20 个已插入的 key 均能命中",
		      "存在已插入的 key 搜索不到!");
	}
	{
		int miss = 0;

		printf("  search key=100 -> %s\n",
		       search(&root, 100) ? "命中(错误!)" : "未命中");
		if (search(&root, 100))
			miss = 1;
		printf("  search key=0   -> %s\n",
		       search(&root, 0) ? "命中(错误!)" : "未命中");
		if (search(&root, 0))
			miss = 1;
		CHECK(!miss, "不存在的 key(100/0) 均未命中",
		      "不存在的 key 被错误命中!");
	}

	printf("\n================ 3. 删除测试 ================\n");
	{
		/* 删除 5 个不同位置的节点: 最小值/中间值/最大值等 */
		int del_keys[] = {1, 5, 10, 15, 20};
		int nd = sizeof(del_keys) / sizeof(del_keys[0]);

		for (i = 0; i < nd; i++) {
			struct my_node *data = search(&root, del_keys[i]);

			if (!data) {
				printf("  erase key=%d -> 节点不存在!\n",
				       del_keys[i]);
				failures++;
				continue;
			}
			rb_erase(&data->node, &root);
			printf("  erase key=%d -> ok\n", del_keys[i]);
		}
	}
	printf("  删除后中序遍历: ");
	print_inorder(&root);
	CHECK(verify(&root), "删除后红黑树性质校验通过",
	      "删除后红黑树性质校验失败!");
	CHECK(count_nodes(&root) == n - 5, "删除后节点数正确(15)",
	      "删除后节点数不正确!");
	{
		int miss_ok = !search(&root, 1) && !search(&root, 10) &&
			      !search(&root, 20);

		CHECK(miss_ok, "已删除的 key(1/10/20) 搜索不到",
		      "已删除的 key 还能被搜索到!");
		for (i = 0; i < n; i++) {
			int was_deleted = (keys[i] == 1 || keys[i] == 5 ||
					   keys[i] == 10 || keys[i] == 15 ||
					   keys[i] == 20);

			if (!was_deleted && !search(&root, keys[i])) {
				printf("  [FAIL] 未删除的 %d 搜索不到了!\n",
				       keys[i]);
				failures++;
			}
		}
	}

	printf("\n================ 4. 删除后重插测试 ================\n");
	/* 把之前删掉的 15 和 5 重新插回去 */
	nodes[0].key = 15;	/* 之前是 key=15 的节点, 已被删除 */
	nodes[9].key = 5;	/* 之前是 key=5 的节点, 已被删除 */
	printf("  reinsert key=15 -> %s\n",
	       insert(&root, &nodes[0]) == 0 ? "ok" : "失败!");
	printf("  reinsert key=5  -> %s\n",
	       insert(&root, &nodes[9]) == 0 ? "ok" : "失败!");
	printf("  重插后中序遍历: ");
	print_inorder(&root);
	CHECK(verify(&root), "重插后红黑树性质校验通过",
	      "重插后红黑树性质校验失败!");

	printf("\n================ 5. 随机压力测试 ================\n");
	{
		/* (i*53+17)%100 是 0~99 的一个伪随机排列, 加 1 后为 1~100 */
		static struct my_node big[100];
		struct rb_root big_root = RB_ROOT;
		int cnt = 0;

		for (i = 0; i < 100; i++) {
			big[i].key = (i * 53 + 17) % 100 + 1;
			if (insert(&big_root, &big[i]) == 0)
				cnt++;
		}
		printf("  乱序插入 100 个节点: 实际插入 %d 个\n", cnt);
		CHECK(cnt == 100 && verify(&big_root),
		      "插入 100 节点后红黑树性质校验通过",
		      "插入 100 节点后校验失败!");

		/* 删除所有 key 为偶数的节点, 共 50 个 */
		for (i = 0; i < 100; i++) {
			if (big[i].key % 2 == 0) {
				rb_erase(&big[i].node, &big_root);
				cnt--;
			}
		}
		printf("  删除所有偶数 key 后剩余节点数: %d\n", cnt);
		CHECK(cnt == 50, "删除后节点数正确(50)",
		      "删除后节点数不正确!");
		CHECK(verify(&big_root), "删除 50 节点后红黑树性质校验通过",
		      "删除 50 节点后校验失败!");
		{
			struct rb_node *node;
			int sorted = 1, prev = 0, first = 1;

			for (node = rb_first(&big_root); node;
			     node = rb_next(node)) {
				struct my_node *d =
					rb_entry(node, struct my_node, node);

				if (!first && d->key <= prev)
					sorted = 0;
				first = 0;
				prev = d->key;
			}
			CHECK(sorted, "100 节点压力测试中序遍历升序",
			      "压力测试中序遍历乱序!");
		}
	}

	printf("\n================ 测试结果 ================\n");
	printf("  当前树节点数: %d\n", count_nodes(&root));
	printf("  中序遍历: ");
	print_inorder(&root);
	if (failures == 0)
		printf("\n>>> 全部测试通过! <<<\n");
	else
		printf("\n>>> 共有 %d 项测试失败! <<<\n", failures);

	return failures ? 1 : 0;
}
