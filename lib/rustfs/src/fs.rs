extern crate alloc;

use crate::api::*;
use alloc::{borrow::ToOwned, string::String, vec::Vec};

#[derive(Default)]
pub struct RustNode {
    pub data: Vec<u8>,
}

pub enum Node {
    Directory(Vec<(String, Node)>),
    Vnode(RustNode),
}

impl RustNode {
    pub fn read(&self) -> &[u8] {
        self.data.as_slice()
    }
    pub fn write(&mut self, io: &UIO) {
        self.data.extend_from_slice(io.as_slice())
    }
}

impl Node {
    pub fn lookup(&self, path: &str) -> Option<&RustNode> {
        let (file, suffix) = path.split_once('/').unwrap_or((path, ""));
        match &self {
            Node::Directory(vec) => vec
                .iter()
                .find(|(name, _)| name == file)
                .and_then(|(_, node)| node.lookup(suffix)),
            Node::Vnode(v) => Some(v),
        }
    }
    pub fn create(&mut self, path: &str) -> Option<&RustNode> {
        let (file, suffix) = path.split_once('/').unwrap_or(("", path));
        match self {
            Node::Vnode(_) => None,
            Node::Directory(vec) => {
                if file.is_empty() && !suffix.is_empty() {
                    vec.push((suffix.to_owned(), Node::Vnode(RustNode::default())));
                    match vec.last() {
                        Some((_, Node::Vnode(rn))) => Some(rn),
                        _ => None,
                    }
                } else {
                    vec.iter_mut()
                        .find(|(name, _)| name == file)
                        .and_then(|(_, node)| node.create(suffix))
                }
            }
        }
    }
    pub fn remove(&mut self, to_remove: &RustNode) -> Option<Node> {
        match self {
            Node::Vnode(_) => None,
            Node::Directory(vec) => {
                let pos = vec.iter().position(|(_, node)| match node {
                    Node::Vnode(v) => {
                        core::ptr::eq(to_remove as *const RustNode, v as *const RustNode)
                    }
                    _ => false,
                });
                if let Some(pos) = pos {
                    Some(vec.swap_remove(pos).1)
                } else {
                    for (_, node) in vec {
                        if let Some(node) = node.remove(to_remove) {
                            return Some(node);
                        }
                    }
                    None
                }
            }
        }
    }
}
