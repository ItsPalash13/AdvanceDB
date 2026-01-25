from math import ceil


NODE_SIZE = 4          # MAX keys in a node
MIN_KEYS = NODE_SIZE // 2


class BPlusNode:
    def __init__(self, is_leaf=False):
        self.is_leaf = is_leaf
        self.keys = []
        self.children = []
        self.next = None   # for leaf nodes
        self.parent = None


class IndexCursor:
    def __init__(self, leaf=None, index=0):
        self.leaf = leaf
        self.index = index

    def valid(self):
        return self.leaf is not None and self.index < len(self.leaf.keys)

    def value(self):
        return self.leaf.keys[self.index]

    def next_tuple(self):
        if self.index + 1 < len(self.leaf.keys):
            self.index += 1
        else:
            self.leaf = self.leaf.next
            self.index = 0


class BPlusTree:
    def __init__(self):
        self.root = BPlusNode(is_leaf=True)

    # ---------------- SEARCH ----------------

    def search(self, key):
        node = self.root
        while not node.is_leaf:
            i = 0
            while i < len(node.keys) and key >= node.keys[i]:
                i += 1
            node = node.children[i]

        for i, k in enumerate(node.keys):
            if k == key:
                return IndexCursor(node, i)
        return None

    def lower_bound(self, key):
        """Return cursor to first key >= given key"""
        node = self.root
        while not node.is_leaf:
            i = 0
            while i < len(node.keys) and key >= node.keys[i]:
                i += 1
            node = node.children[i]

        for i, k in enumerate(node.keys):
            if k >= key:
                return IndexCursor(node, i)

        return IndexCursor(node.next, 0) if node.next else None

    # ---------------- INSERT ----------------

    def insert(self, key):
        if not isinstance(key, int):
            raise TypeError("Only integers allowed")

        leaf = self._find_leaf(self.root, key)
        self._insert_into_leaf(leaf, key)

        if len(leaf.keys) > NODE_SIZE:
            self._split_leaf(leaf)

    def _find_leaf(self, node, key):
        while not node.is_leaf:
            i = 0
            while i < len(node.keys) and key >= node.keys[i]:
                i += 1
            node = node.children[i]
        return node

    def _insert_into_leaf(self, leaf, key):
        leaf.keys.append(key)
        leaf.keys.sort()

    # ---------------- SPLIT ----------------

    def _split_leaf(self, leaf):
        new_leaf = BPlusNode(is_leaf=True)
        mid = ceil(len(leaf.keys) / 2)

        new_leaf.keys = leaf.keys[mid:]
        leaf.keys = leaf.keys[:mid]

        new_leaf.next = leaf.next
        leaf.next = new_leaf

        new_leaf.parent = leaf.parent

        promoted_key = new_leaf.keys[0]
        self._insert_into_parent(leaf, promoted_key, new_leaf)

    def _split_internal(self, node):
        new_internal = BPlusNode(is_leaf=False)
        mid = len(node.keys) // 2

        promoted_key = node.keys[mid]

        new_internal.keys = node.keys[mid + 1:]
        new_internal.children = node.children[mid + 1:]

        for c in new_internal.children:
            c.parent = new_internal

        node.keys = node.keys[:mid]
        node.children = node.children[:mid + 1]

        new_internal.parent = node.parent

        self._insert_into_parent(node, promoted_key, new_internal)

    def _insert_into_parent(self, left, key, right):
        if left.parent is None:
            new_root = BPlusNode(is_leaf=False)
            new_root.keys = [key]
            new_root.children = [left, right]
            left.parent = right.parent = new_root
            self.root = new_root
            return

        parent = left.parent
        idx = parent.children.index(left)

        parent.keys.insert(idx, key)
        parent.children.insert(idx + 1, right)
        right.parent = parent

        if len(parent.keys) > NODE_SIZE:
            self._split_internal(parent)

    # ---------------- DELETE (MERGE @ 50%) ----------------

    def delete(self, key):
        leaf = self._find_leaf(self.root, key)
        if key not in leaf.keys:
            return

        leaf.keys.remove(key)

        if leaf is self.root:
            return

        if len(leaf.keys) < MIN_KEYS:
            self._rebalance(leaf)

    def _rebalance(self, node):
        parent = node.parent
        idx = parent.children.index(node)

        left = parent.children[idx - 1] if idx > 0 else None
        right = parent.children[idx + 1] if idx < len(parent.children) - 1 else None

        # Try redistribution
        if left and len(left.keys) > MIN_KEYS:
            node.keys.insert(0, left.keys.pop())
            parent.keys[idx - 1] = node.keys[0]
            return

        if right and len(right.keys) > MIN_KEYS:
            node.keys.append(right.keys.pop(0))
            parent.keys[idx] = right.keys[0]
            return

        # Merge
        if left:
            left.keys.extend(node.keys)
            left.next = node.next
            parent.keys.pop(idx - 1)
            parent.children.pop(idx)
        else:
            node.keys.extend(right.keys)
            node.next = right.next
            parent.keys.pop(idx)
            parent.children.pop(idx + 1)

        if parent is self.root and not parent.keys:
            self.root = parent.children[0]
            self.root.parent = None
        elif parent and len(parent.keys) < MIN_KEYS:
            self._rebalance(parent)

    # ---------------- DEBUG ----------------

    def print_tree(self):
        level = [self.root]
        while level:
            next_level = []
            for node in level:
                print(node.keys, end="  ")
                if not node.is_leaf:
                    next_level.extend(node.children)
            print()
            level = next_level

tree = BPlusTree()

for x in [10, 20, 5, 6, 12, 30, 7, 17]:
    tree.insert(x)

tree.print_tree()
print("--------------------------------")
cursor = tree.lower_bound(6)
while cursor and cursor.valid():
    print(cursor.value())
    cursor.next_tuple()
