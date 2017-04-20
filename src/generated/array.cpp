Array_VkDescriptorSetLayout array_create_VkDescriptorSetLayout(Allocator * allocator)
{
	Array_VkDescriptorSetLayout a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkDescriptorSetLayout array_create_VkDescriptorSetLayout(Allocator * allocator, isize capacity)
{
	Array_VkDescriptorSetLayout a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkDescriptorSetLayout *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkDescriptorSetLayout *a, VkDescriptorSetLayout e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkDescriptorSetLayout *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkDescriptorSetLayoutBinding array_create_VkDescriptorSetLayoutBinding(Allocator * allocator)
{
	Array_VkDescriptorSetLayoutBinding a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkDescriptorSetLayoutBinding array_create_VkDescriptorSetLayoutBinding(Allocator * allocator, isize capacity)
{
	Array_VkDescriptorSetLayoutBinding a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkDescriptorSetLayoutBinding *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkDescriptorSetLayoutBinding *a, VkDescriptorSetLayoutBinding e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkDescriptorSetLayoutBinding *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkVertexInputBindingDescription array_create_VkVertexInputBindingDescription(Allocator * allocator)
{
	Array_VkVertexInputBindingDescription a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkVertexInputBindingDescription array_create_VkVertexInputBindingDescription(Allocator * allocator, isize capacity)
{
	Array_VkVertexInputBindingDescription a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkVertexInputBindingDescription *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkVertexInputBindingDescription *a, VkVertexInputBindingDescription e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkVertexInputBindingDescription *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkVertexInputAttributeDescription array_create_VkVertexInputAttributeDescription(Allocator * allocator)
{
	Array_VkVertexInputAttributeDescription a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkVertexInputAttributeDescription array_create_VkVertexInputAttributeDescription(Allocator * allocator, isize capacity)
{
	Array_VkVertexInputAttributeDescription a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkVertexInputAttributeDescription *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkVertexInputAttributeDescription *a, VkVertexInputAttributeDescription e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkVertexInputAttributeDescription *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkPipelineShaderStageCreateInfo array_create_VkPipelineShaderStageCreateInfo(Allocator * allocator)
{
	Array_VkPipelineShaderStageCreateInfo a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkPipelineShaderStageCreateInfo array_create_VkPipelineShaderStageCreateInfo(Allocator * allocator, isize capacity)
{
	Array_VkPipelineShaderStageCreateInfo a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkPipelineShaderStageCreateInfo *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkPipelineShaderStageCreateInfo *a, VkPipelineShaderStageCreateInfo e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkPipelineShaderStageCreateInfo *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkDescriptorPoolSize array_create_VkDescriptorPoolSize(Allocator * allocator)
{
	Array_VkDescriptorPoolSize a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkDescriptorPoolSize array_create_VkDescriptorPoolSize(Allocator * allocator, isize capacity)
{
	Array_VkDescriptorPoolSize a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkDescriptorPoolSize *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkDescriptorPoolSize *a, VkDescriptorPoolSize e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkDescriptorPoolSize *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkAttachmentDescription array_create_VkAttachmentDescription(Allocator * allocator)
{
	Array_VkAttachmentDescription a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkAttachmentDescription array_create_VkAttachmentDescription(Allocator * allocator, isize capacity)
{
	Array_VkAttachmentDescription a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkAttachmentDescription *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkAttachmentDescription *a, VkAttachmentDescription e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkAttachmentDescription *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_VkAttachmentReference array_create_VkAttachmentReference(Allocator * allocator)
{
	Array_VkAttachmentReference a  = {};
	a.allocator = allocator;

	return a;
}

Array_VkAttachmentReference array_create_VkAttachmentReference(Allocator * allocator, isize capacity)
{
	Array_VkAttachmentReference a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_VkAttachmentReference *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_VkAttachmentReference *a, VkAttachmentReference e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_VkAttachmentReference *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_Entity array_create_Entity(Allocator * allocator)
{
	Array_Entity a  = {};
	a.allocator = allocator;

	return a;
}

Array_Entity array_create_Entity(Allocator * allocator, isize capacity)
{
	Array_Entity a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_Entity *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_Entity *a, Entity e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_Entity *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_Vector3 array_create_Vector3(Allocator * allocator)
{
	Array_Vector3 a  = {};
	a.allocator = allocator;

	return a;
}

Array_Vector3 array_create_Vector3(Allocator * allocator, isize capacity)
{
	Array_Vector3 a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_Vector3 *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_Vector3 *a, Vector3 e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_Vector3 *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_i32 array_create_i32(Allocator * allocator)
{
	Array_i32 a  = {};
	a.allocator = allocator;

	return a;
}

Array_i32 array_create_i32(Allocator * allocator, isize capacity)
{
	Array_i32 a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_i32 *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_i32 *a, i32 e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_i32 *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_RenderObject array_create_RenderObject(Allocator * allocator)
{
	Array_RenderObject a  = {};
	a.allocator = allocator;

	return a;
}

Array_RenderObject array_create_RenderObject(Allocator * allocator, isize capacity)
{
	Array_RenderObject a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_RenderObject *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_RenderObject *a, RenderObject e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_RenderObject *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

Array_IndexRenderObject array_create_IndexRenderObject(Allocator * allocator)
{
	Array_IndexRenderObject a  = {};
	a.allocator = allocator;

	return a;
}

Array_IndexRenderObject array_create_IndexRenderObject(Allocator * allocator, isize capacity)
{
	Array_IndexRenderObject a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

void array_destroy(Array_IndexRenderObject *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

isize array_add(Array_IndexRenderObject *a, IndexRenderObject e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

isize array_remove(Array_IndexRenderObject *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

