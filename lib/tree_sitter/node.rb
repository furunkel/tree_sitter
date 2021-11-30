require 'tree_sitter/core'

module TreeSitter
  class Node
    def cursor
      Tree::Cursor.new self
    end

    def inspect
      self.text || "<empty>"
    end
  end
end